#include "spcd.h"

MODULE_LICENSE("GPL");

static void reset(void);
static void reset_stats(void);
static void print_stats(void);

unsigned long pt_pf;
unsigned long pt_addr_conflict;
unsigned long pt_pf_extra;
unsigned long pt_num_walks;

struct task_struct *pt_task;

static struct task_struct *pt_thread;

unsigned long pt_pte_fixes;

struct page* (*vm_normal_page_p)(struct vm_area_struct *vma, unsigned long addr, pte_t pte);

int (*walk_page_range_p)(unsigned long addr, unsigned long end,
			struct mm_walk *walk);


static inline void fix_pte(pmd_t *pmd, pte_t *pte)
{
	if (!pte_present(*pte) && !pte_none(*pte)) {
		*pte = pte_set_flags(*pte, _PAGE_PRESENT);
		pt_pte_fixes++;
	}
}


void spcd_pte_fault_handler(struct mm_struct *mm,
			struct vm_area_struct *vma, unsigned long address,
			pte_t *pte, pmd_t *pmd, unsigned int flags)
{
	int tid;

	if (!pt_task || pt_task->mm != mm)
		jprobe_return();

	fix_pte(pmd, pte);
	pt_pf++;

	tid = pt_get_tid(current->pid);
	if (tid > -1){
		pt_check_comm(tid, address);
	}

	jprobe_return();
}


void spcd_del_page_handler(struct page *page)
{
	if (page_mapped(page))
		atomic_set(&(page)->_mapcount, -1);

	jprobe_return();
}

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
		//printk("hit: %lu\n", pte_val(*pte));
		fix_pte(pmd, pte);
	} while (pte++, addr += PAGE_SIZE, addr != end);

	pte_unmap_unlock(start_pte, ptl);
	return addr;
}

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
			reset();
			printk("pt: stop %s (pid %d)\n", task->comm, task->pid);
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

	if (pt_task || ret)
		return 0;

	if (spcd_check_name(task->comm)) {
		printk("\npt: start %s (pid %d)\n", task->comm, task->pid);
		pt_add_pid(task->pid);
		pt_task = task;
	}

	return 0;
}


int spcd_fork_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int pid = regs_return_value(regs);
	struct pid *pids;
	struct task_struct *task = NULL;

	if (!pt_task)
		return 0;
	
	rcu_read_lock();
	pids = find_vpid(pid);
	if (pids)
		task = pid_task(pids, PIDTYPE_PID);
	rcu_read_unlock();

	if (!task)
		return 0;

	if (pt_task->parent->pid == task->parent->pid)
		pt_add_pid(pid);

	return 0;
}


int init_module(void)
{
	printk("Welcome.....\n");
	reset_stats();

	register_probes();

	vm_normal_page_p = (void*) kallsyms_lookup_name("vm_normal_page");
	walk_page_range_p = (void*) kallsyms_lookup_name("walk_page_range");

	pt_thread = kthread_create(pt_pf_thread_func, NULL, "pt_pf_thread");
	wake_up_process(pt_thread);

	return 0;
}


void cleanup_module(void)
{
	if (pt_thread)
		kthread_stop(pt_thread);

	unregister_probes();

	printk("Bye.....\n");
}


static inline void reset(void)
{
	pt_task = NULL;
}


static inline void reset_stats(void)
{
	pt_pid_clear();
	pt_mem_clear();
	pt_pte_fixes = 0;
	pt_pf = 0;
	pt_share_clear();
	spcd_pf_thread_clear();
}


static inline void print_stats(void)
{
	int nt = spcd_get_num_threads();

	printk("(%d threads): %lu pfs (%lu extra, %lu fixes), %lu walks, %lu addr conflicts\n", nt, pt_pf, pt_pf_extra, pt_pte_fixes, pt_num_walks, pt_addr_conflict);

	pt_print_share();
}
