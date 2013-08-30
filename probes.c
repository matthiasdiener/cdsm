#include "spcd.h"

#include <linux/kprobes.h>

unsigned long spcd_pf;
unsigned long spcd_pte_fixes;
int spcd_vma_shared_flag = 1;


void reset_stats(void)
{
	spcd_pid_clear();
	spcd_mem_init();
	spcd_pte_fixes = 0;
	spcd_pf = 0;
	spcd_vma_shared_flag = 1;
	spcd_comm_init();
	spcd_pf_thread_init();
}


static inline
void print_stats(void)
{
	int nt = spcd_get_num_threads();

	printk("(%d threads): %lu pfs (%lu extra, %lu fixes), %lu addr conflicts\n", nt, spcd_pf, spcd_pf_extra, spcd_pte_fixes, spcd_addr_conflict);

	spcd_print_comm();
}


static inline
int check_name(char *name)
{
	int len = strlen(name);

	/* Only programs whose name ends with ".x" are accepted */
	if (name[len-2] == '.' && name[len-1] == 'x')
		return 1;

	return 0;
}


static inline
void fix_pte_empty(pmd_t *pmd, pte_t *pte)
{
	/* Empty */
}


static inline
void fix_pte_real(pmd_t *pmd, pte_t *pte)
{
	if (!pte_present(*pte) && !pte_none(*pte)) {
		*pte = pte_set_flags(*pte, _PAGE_PRESENT);
		spcd_pte_fixes++;
	}
}

static
void (*fix_pte)(pmd_t *pmd, pte_t *pte) = fix_pte_empty;

static
void pte_fault_handler(struct mm_struct *mm,
							struct vm_area_struct *vma, unsigned long address,
							pte_t *pte, pmd_t *pmd, unsigned int flags)
{
	int pid = current->pid;
	int tid = spcd_get_tid(pid);

	/* TODO: check if we can move this into the next if */
	fix_pte(pmd, pte);

	if (tid != -1){
		unsigned long physaddr = pte_pfn(*pte);
		spcd_pf++;

		/* TODO: make the physaddr calculation later so it never is zero */
		if (physaddr)
			spcd_check_comm(tid, (physaddr<<PAGE_SHIFT) | (address & (PAGE_SIZE-1)));
	}

	jprobe_return();
}


static
void del_page_handler(struct page *page)
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

static
void unmap_page_range_handler(struct mmu_gather *tlb,
								   struct vm_area_struct *vma,
								   unsigned long addr, unsigned long end,
								   struct zap_details *details)
{
	pgd_t *pgd;
	unsigned long next;

	if (!check_name(current->comm))
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


static
void process_handler(struct task_struct *tsk)
{
	int at, tid = spcd_get_tid(tsk->pid);

	if (tid > -1 && (tsk->flags & PF_EXITING)) {
		spcd_delete_pid(tsk->pid);
		at = spcd_get_active_threads();
		printk("SPCD: %s stop (pid %d, tid %d), #active: %d\n", tsk->comm, tsk->pid, tid, at);
		if (at == 0) {
			printk("SPCD: stop app %s (pid %d, tid %d)\n", tsk->comm, tsk->pid, tid);
			print_stats();
			reset_stats();
		}
		jprobe_return();
	}

	if (check_name(tsk->comm) && tid == -1 && !(tsk->flags & PF_EXITING)) {
		tid = spcd_add_pid(tsk->pid);
		printk("SPCD: new process %s (pid %d, tid %d); #active: %d\n", tsk->comm, tsk->pid, tid, spcd_get_active_threads());
	}

	jprobe_return();
}


static
int thread_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int pid = regs_return_value(regs);

	if (check_name(current->comm) && pid > 0) {
		int tid = spcd_add_pid(pid);
		spcd_vma_shared_flag = 0;
		printk("SPCD: new thread %s (pid:%d, tid:%d); #active: %d\n", current->comm, pid, tid, spcd_get_active_threads());
	}

	return 0;
}



static struct jprobe pte_fault_probe = {
	.entry = pte_fault_handler,
	.kp.symbol_name = "handle_pte_fault",
};

static struct kretprobe thread_probe = {
	.handler = thread_handler,
	.kp.symbol_name = "do_fork",
};

static struct jprobe process_probe = {
	.entry = process_handler,
	.kp.symbol_name = "acct_update_integrals",
};

static struct jprobe del_page_probe = {
	.entry = del_page_handler,
	.kp.symbol_name = "__delete_from_page_cache",
};

static struct jprobe unmap_page_range_probe = {
	.entry = unmap_page_range_handler,
	.kp.symbol_name = "unmap_page_range",
};


void spcd_probes_init(void)
{
	int ret;

	if ((ret=register_jprobe(&pte_fault_probe))) {
		printk("SPCD BUG: handle_pte_fault missing, %d\n", ret);
	}
	if ((ret=register_jprobe(&process_probe))){
		printk("SPCD BUG: acct_update_integrals missing, %d\n", ret);
	}
	if ((ret=register_kretprobe(&thread_probe))){
		printk("SPCD BUG: do_fork missing, %d\n", ret);
	}

	if (do_pf) {
		fix_pte = fix_pte_real;

		if ((ret=register_jprobe(&del_page_probe))){
			printk("SPCD BUG: __delete_from_page_cache missing, %d\n", ret);
		}
		if ((ret=register_jprobe(&unmap_page_range_probe))){
			printk("SPCD BUG: unmap_page_range missing, %d\n", ret);
		}
	}
}


void spcd_probes_cleanup(void)
{
	unregister_jprobe(&pte_fault_probe);
	unregister_jprobe(&process_probe);
	unregister_kretprobe(&thread_probe);
	if (do_pf) {
		unregister_jprobe(&del_page_probe);
		unregister_jprobe(&unmap_page_range_probe);
	}
}
