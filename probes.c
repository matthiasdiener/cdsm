#include "spcd.h"

unsigned long spcd_pf;
unsigned long spcd_pte_fixes;
int spcd_vma_shared_flag = 1;


void reset_stats(void)
{
	spcd_pid_clear();
	spcd_mem_clear();
	spcd_pte_fixes = 0;
	spcd_pf = 0;
	spcd_vma_shared_flag = 1;
	spcd_share_clear();
	spcd_pf_thread_clear();
}


static inline
void print_stats(void)
{
	int nt = spcd_get_num_threads();

	printk("(%d threads): %lu pfs (%lu extra, %lu fixes), %lu addr conflicts\n", nt, spcd_pf, spcd_pf_extra, spcd_pte_fixes, spcd_addr_conflict);

	spcd_print_share();
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
void fix_pte(pmd_t *pmd, pte_t *pte)
{
#ifdef ENABLE_EXTRA_PF
	if (!pte_present(*pte) && !pte_none(*pte)) {
		*pte = pte_set_flags(*pte, _PAGE_PRESENT);
		spcd_pte_fixes++;
	}
#endif
}


void spcd_pte_fault_handler(struct mm_struct *mm,
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

#ifdef ENABLE_EXTRA_PF
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

	if (spcd_get_tid(current->pid) == -1)
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
#endif

void spcd_exit_process_handler(struct task_struct *task)
{
	int pid = task->pid;
	int tid = spcd_get_tid(pid);
	int at;

	if (tid > -1) {
		spcd_delete_pid(pid);
		at = spcd_get_active_threads();
		printk("SPCD: %s stop (pid %d, tid %d), #active: %d\n", task->comm, pid, tid, at);
		if (at == 0) {
			printk("SPCD: stop app %s (pid %d, tid %d)\n", task->comm, pid, tid);
			print_stats();
			reset_stats();
		}
	}

	jprobe_return();
}

static
int spcd_new_process_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct task_struct *task = current;

	if (check_name(task->comm)) {
		int tid = spcd_add_pid(task->pid);
		printk("SPCD: new process %s (pid %d, tid %d); #active: %d\n", task->comm, task->pid, tid, spcd_get_active_threads());
	}

	return 0;
}

static
int spcd_fork_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    int pid = regs_return_value(regs);
    struct pid *pids;
    struct task_struct *task = NULL;

    if (spcd_get_active_threads()==0)
        return 0;

    rcu_read_lock();
    pids = find_vpid(pid);
    if (pids)
        task = pid_task(pids, PIDTYPE_PID);
    rcu_read_unlock();

    if (!task)
        return 0;


	if (check_name(task->comm)) {
		int tid = spcd_add_pid(task->pid);
		spcd_vma_shared_flag = 0;
		printk("SPCD: new thread %s (pid:%d, tid:%d); #active: %d\n", task->comm, task->pid, tid, spcd_get_active_threads());
	}

	return 0;
}



static struct jprobe spcd_pte_fault_probe = {
	.entry = spcd_pte_fault_handler,
	.kp.symbol_name = "handle_pte_fault",
};

static struct kretprobe spcd_fork_probe = {
	.handler = spcd_fork_handler,
	.kp.symbol_name = "do_fork",
};


static struct jprobe spcd_exit_process_probe = {
	.entry = spcd_exit_process_handler,
	.kp.symbol_name = "perf_event_exit_task",
};

static struct kretprobe spcd_new_process_probe = {
	.handler = spcd_new_process_handler,
	.kp.symbol_name = "do_execve",
};

#ifdef ENABLE_EXTRA_PF
static struct jprobe spcd_del_page_probe = {
	.entry = spcd_del_page_handler,
	.kp.symbol_name = "__delete_from_page_cache",
};

static struct jprobe spcd_unmap_page_range_probe = {
	.entry = spcd_unmap_page_range_handler,
	.kp.symbol_name = "unmap_page_range",
};
#endif

void register_probes(void)
{
	int ret;
	if ((ret=register_jprobe(&spcd_pte_fault_probe))) {
		printk("SPCD BUG: handle_pte_fault missing, %d\n", ret);
	}
	if ((ret=register_jprobe(&spcd_exit_process_probe))){
		printk("SPCD BUG: perf_event_exit_task missing, %d\n", ret);
	}
	if ((ret=register_kretprobe(&spcd_new_process_probe))){
		printk("SPCD BUG: do_execve missing, %d\n", ret);
	}
	if ((ret=register_kretprobe(&spcd_fork_probe))){
		printk("SPCD BUG: do_fork missing, %d\n", ret);
	}
#ifdef ENABLE_EXTRA_PF
	if ((ret=register_jprobe(&spcd_del_page_probe))){
		printk("SPCD BUG: __delete_from_page_cache missing, %d\n", ret);
	}
	if ((ret=register_jprobe(&spcd_unmap_page_range_probe))){
		printk("SPCD BUG: unmap_page_range missing, %d\n", ret);
	}
#endif
}


void unregister_probes(void)
{
	unregister_jprobe(&spcd_pte_fault_probe);
	unregister_jprobe(&spcd_exit_process_probe);
	unregister_kretprobe(&spcd_new_process_probe);
	unregister_kretprobe(&spcd_fork_probe);
#ifdef ENABLE_EXTRA_PF
	unregister_jprobe(&spcd_del_page_probe);
	unregister_jprobe(&spcd_unmap_page_range_probe);
#endif
}
