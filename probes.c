#include "spcd.h"

unsigned long pt_pf;
unsigned long pt_pte_fixes;


void reset_stats(void)
{
	pt_pid_clear();
	pt_mem_clear();
	pt_pte_fixes = 0;
	pt_pf = 0;
	pt_share_clear();
	spcd_pf_thread_clear();
}


static inline
void print_stats(void)
{
	int nt = spcd_get_num_threads();

	printk("(%d threads): %lu pfs (%lu extra, %lu fixes), %lu walks, %lu addr conflicts\n", nt, pt_pf, pt_pf_extra, pt_pte_fixes, pt_num_walks, pt_addr_conflict);

	pt_print_share();
}


static inline
int check_name(char *name)
{
	const char *bm_names[] = {"bt.", "cg.", "dc.", "dt.", "ep.", "ft.", "is.", "lu.", "mg.", "sp.", "ua.", /*NAS*/ 
	"blackscholes", "bodytrack", "facesim", "ferret", "freqmine", "rtview", "swaptions", "fluidanimate", "vips", "x264", "canneal", "dedup", "streamcluster", /*Parsec*/
	"LU", "FFT", "CHOLESKY", /*Splash2*/
	"wupwise_", "swim_", "mgrid_", "applu_", "galgel_", "equake_", "apsi_", "gafort_", "fma3d_", "art_", "ammp_", /* Spec OMP 2001 */
	"md_omp_", "bwaves_", "nabmd_", "bt_", "bots-alignment_", "bots-sparselu_", "ilbdc_", /*"fma3d", "swim_", */ "convert_", "mg_", "lu_", "smithwaterman_", "kdtree_", /* Spec OMP 2012 */
	};

	int i, len = sizeof(bm_names)/sizeof(bm_names[0]);

	for (i=0; i<len; i++) {
		if (strstr(name, bm_names[i]))
			return 1;
	}

	return 0;
}


static inline
void fix_pte(pmd_t *pmd, pte_t *pte)
{
	if (!pte_present(*pte) && !pte_none(*pte)) {
		*pte = pte_set_flags(*pte, _PAGE_PRESENT);
		pt_pte_fixes++;
	}
}


static inline
int is_shared(struct vm_area_struct *vma)
{
	if (!vma)
		return 0;
	return vma->vm_flags & VM_SHARED ? 1 : 0;
}


void spcd_pte_fault_handler(struct mm_struct *mm,
							struct vm_area_struct *vma, unsigned long address,
							pte_t *pte, pmd_t *pmd, unsigned int flags)
{
	int tid;

	fix_pte(pmd, pte);

	tid = pt_get_tid(current->pid);
	if (tid > -1){
		pt_pf++;
		if (is_shared(find_vma(mm, address))) {
			unsigned long physaddr = pte_pfn(*pte);
			pt_check_comm(tid, physaddr<<(PAGE_SHIFT) );
			// pt_check_comm(tid, (physaddr<<PAGE_SHIFT) | (address & (PAGE_SIZE-1)));
			printk("addr:%lx physaddr: %lx\n", address, physaddr);
		}
	}

	jprobe_return();
}


void spcd_del_page_handler(struct page *page)
{
	if (page_mapped(page))
		atomic_set(&(page)->_mapcount, -1);

	jprobe_return();
}


static
unsigned long zap_pte_range(struct mmu_gather *tlb,
							struct vm_area_struct *vma, pmd_t *pmd,
							unsigned long addr, unsigned long end,
							struct zap_details *details)
{
	struct mm_struct *mm = tlb->mm;
	pte_t *start_pte, *pte;
	spinlock_t *ptl;

	start_pte = pte_offset_map_lock(mm, pmd, addr, &ptl);
	pte = start_pte;

	do {
		fix_pte(pmd, pte);
	} while (pte++, addr += PAGE_SIZE, addr != end);

	pte_unmap_unlock(start_pte, ptl);
	return addr;
}


static
unsigned long zap_pmd_range(struct mmu_gather *tlb,
							struct vm_area_struct *vma, pud_t *pud,
							unsigned long addr, unsigned long end,
							struct zap_details *details)
{
	pmd_t *pmd;
	unsigned long next;

	pmd = pmd_offset(pud, addr);
	do {
		next = pmd_addr_end(addr, end);
		if (pmd_none(*pmd))
			continue;
		next = zap_pte_range(tlb, vma, pmd, addr, next, details);
	} while (pmd++, addr = next, addr != end);
	return addr;
}


static
unsigned long zap_pud_range(struct mmu_gather *tlb,
							struct vm_area_struct *vma, pgd_t *pgd,
							unsigned long addr, unsigned long end,
							struct zap_details *details)
{
	pud_t *pud;
	unsigned long next;

	pud = pud_offset(pgd, addr);
	do {
		next = pud_addr_end(addr, end);
		if (pud_none(*pud))
			continue;
		next = zap_pmd_range(tlb, vma, pud, addr, next, details);
	} while (pud++, addr = next, addr != end);

	return addr;
}


void spcd_unmap_page_range_handler(struct mmu_gather *tlb,
								   struct vm_area_struct *vma,
								   unsigned long addr, unsigned long end,
								   struct zap_details *details)
{
	pgd_t *pgd;
	unsigned long next;

	if (pt_get_tid(current->pid) == -1)
		jprobe_return();

	pgd = pgd_offset(vma->vm_mm, addr);
	do {
		next = pgd_addr_end(addr, end);
		if (pgd_none(*pgd))
			continue;
		next = zap_pud_range(tlb, vma, pgd, addr, next, details);
	} while (pgd++, addr = next, addr != end);

	jprobe_return();
}


void spcd_exit_process_handler(struct task_struct *task)
{
	int tid = pt_get_tid(task->pid);

	if (tid > -1) {
		pt_delete_pid(task->pid);
		if (spcd_get_active_threads() == 0) {
			printk("SPCD: stop %s (pid %d)\n", task->comm, task->pid);
			print_stats();
			reset_stats();
		}
	}

	jprobe_return();
}


int spcd_new_process_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int ret = regs_return_value(regs);
	struct task_struct *task = current;

	if (ret)
		return 0;

	if (check_name(task->comm)) {
		printk("SPCD: start %s (pid %d)\n", task->comm, task->pid);
		pt_add_pid(task->pid);
		// pt_task = task;
	}

	return 0;
}



static struct jprobe spcd_pte_fault_jprobe = {
	.entry = spcd_pte_fault_handler,
	.kp.symbol_name = "handle_pte_fault",
};

static struct jprobe spcd_exit_process_probe = {
	.entry = spcd_exit_process_handler,
	.kp.symbol_name = "perf_event_exit_task",
};

static struct kretprobe spcd_new_process_probe = {
	.handler = spcd_new_process_handler,
	.kp.symbol_name = "do_execve",
};

static struct jprobe spcd_del_page_probe = {
	.entry = spcd_del_page_handler,
	.kp.symbol_name = "__delete_from_page_cache",
};

static struct jprobe spcd_unmap_page_range_probe = {
	.entry = spcd_unmap_page_range_handler,
	.kp.symbol_name = "unmap_page_range",
};


void register_probes(void)
{
	int ret;
	if ((ret=register_jprobe(&spcd_pte_fault_jprobe))) {
		printk("SPCD BUG: handle_pte_fault missing, %d\n", ret);
	}
	if ((ret=register_jprobe(&spcd_exit_process_probe))){
		printk("SPCD BUG: perf_event_exit_task missing, %d\n", ret);
	}
	if ((ret=register_kretprobe(&spcd_new_process_probe))){
		printk("SPCD BUG: do_execve missing, %d\n", ret);
	}
	if ((ret=register_jprobe(&spcd_del_page_probe))){
		printk("SPCD BUG: __delete_from_page_cache missing, %d\n", ret);
	}
	if ((ret=register_jprobe(&spcd_unmap_page_range_probe))){
		printk("SPCD BUG: unmap_page_range missing, %d\n", ret);
	}
}


void unregister_probes(void)
{
	unregister_jprobe(&spcd_pte_fault_jprobe);
	unregister_jprobe(&spcd_exit_process_probe);
	unregister_kretprobe(&spcd_new_process_probe);
	unregister_jprobe(&spcd_del_page_probe);
	unregister_jprobe(&spcd_unmap_page_range_probe);
}
