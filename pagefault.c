#include "spcd.h"

struct pf_process_t {
	struct vm_area_struct * vma;
	unsigned long next_addr;
	unsigned num_walks;
};

static struct pf_process_t* proc = NULL;

extern int num_faults;

unsigned long pt_pf_extra;

static void pf_pagewalk(long pid, struct mm_struct *mm);

static int (*walk_page_range_p)(unsigned long addr, unsigned long end,
								struct mm_walk *walk) = NULL;


int spcd_pagefault_func(void* v)
{
	int i;
	while (1) {
		msleep(100);
		if (kthread_should_stop())
			return 0;

		if (spcd_get_active_threads() < 4) {
			// pt_pf_pagewalk(pt_task->mm);
			continue;
		}
		for (i=0; i<spcd_get_active_threads(); i++) {
			struct task_struct *tsk = pid_task(find_vpid(pt_get_pid(i)), PIDTYPE_PID);
			// printk("pagewalk thread %d, pid %d, tsk %p\n", i, pt_get_pid(i), tsk);
			if (tsk)
				pf_pagewalk(i, tsk->mm);

		}

	}

}


static inline
int callback_page_walk(pte_t *pte, unsigned long addr, unsigned long next_addr, struct mm_walk *walk)
{

	if (pte_none(*pte)
		|| !pte_present(*pte)
		// || !pte_young(*pte)
		// || pte_special(*pte)
		)
		return 0;

	proc[(long)walk->private].next_addr = addr;

	*pte = pte_clear_flags(*pte, _PAGE_PRESENT);

	pt_pf_extra++;

	return 1;
}


static inline
int is_writable(struct vm_area_struct *vma)
{
	return vma->vm_flags & VM_WRITE ? 1 : 0;
}


static inline
int is_shared(struct vm_area_struct *vma)
{
	return vma->vm_flags & VM_SHARED ? 1 : 0;
}

/*
static pte_t *walk_page_table(struct mm_struct *mm, unsigned long addr)
{
	pgd_t *pgd;
	pte_t *ptep = NULL;
	pud_t *pud;
	pmd_t *pmd;

	pgd = pgd_offset(mm, addr);
	if (pgd_none(*pgd) || pgd_bad(*pgd))
		goto out;

	pud = pud_offset(pgd, addr);
	if (pud_none(*pud) || pud_bad(*pud))
		goto out;

	pmd = pmd_offset(pud, addr);
	if (pmd_none(*pmd) || pmd_bad(*pmd))
		goto out;

	ptep = pte_offset_map(pmd, addr);
out:
	return ptep;

}
*/

static inline
struct vm_area_struct* find_next_vma(long pid, struct mm_struct *mm, struct vm_area_struct* prev_vma)
{
	struct vm_area_struct *tmp = prev_vma;
	int i = 0;

	while (1) {

		if (!tmp) {
			if (++i>2)
				return NULL;
			proc[pid].num_walks++;
			tmp = mm->mmap;
		} else {
			tmp = tmp->vm_next;
		}

		if (tmp && is_shared(tmp)) {
			// pte_t *pte = walk_page_table(mm, tmp->vm_start);
			// unsigned long physaddr = pte ? pte_pfn(*pte) : 0;
			// printk("pid: %d, vma: %lx, size: %lu physaddr: %lx\n", mm->owner->pid, tmp->vm_start, (tmp->vm_end-tmp->vm_start)/1024, physaddr);
			return tmp;
		}
	}
}


static
void pf_pagewalk(long pid, struct mm_struct *mm)
{
	int i;
	struct mm_walk walk = {
		.pte_entry = callback_page_walk,
		.mm = mm,
		.private = (void*) pid,
	};

	if (!mm)
		return;

	down_write(&mm->mmap_sem);

	for (i = 0; i < num_faults; i++) {
		unsigned addr_pbit_changed = 0;
		unsigned long start = proc[pid].num_walks;

		if (spcd_get_active_threads() < 4)
			goto out;

		if (proc[pid].vma == NULL) {
			proc[pid].vma = find_next_vma(pid, mm, NULL);
			if (!proc[pid].vma) goto out;
			proc[pid].next_addr = proc[pid].vma->vm_start;
		}

		while (addr_pbit_changed == 0) {

			if ((proc[pid].num_walks-start)>2)
				goto out;

			addr_pbit_changed = (*walk_page_range_p)(proc[pid].next_addr, proc[pid].vma->vm_end, &walk);

			if (addr_pbit_changed) {
				proc[pid].next_addr += PAGE_SIZE*((get_cycles()%61) + 1); //Magic
				if (proc[pid].next_addr >= proc[pid].vma->vm_end) {
					proc[pid].vma = find_next_vma(pid, mm, proc[pid].vma);
					if (!proc[pid].vma) goto out;
					proc[pid].next_addr = proc[pid].vma->vm_start;
				}
			}
			else {
				proc[pid].vma = find_next_vma(pid, mm, proc[pid].vma);
				if (!proc[pid].vma) goto out;
				proc[pid].next_addr = proc[pid].vma->vm_start;
			}
		}

	}
out:
	// printk("pagewalk thread %ld done, %d faults\n", pid, i);
	up_write(&mm->mmap_sem);
}


void spcd_pf_thread_clear(void)
{
	pt_pf_extra = 0;
	if (!proc)
		proc = kmalloc(max_threads * sizeof (struct pf_process_t), GFP_KERNEL);
	if (!proc)
		printk("SPCD BUG: could not allocate proc\n");
	if (proc)
		memset(proc, 0, max_threads * sizeof (struct pf_process_t));

	if (!walk_page_range_p) {
		walk_page_range_p = (void*) kallsyms_lookup_name("walk_page_range");
		if (!walk_page_range_p)
			printk("SPCD BUG: walk_page_range missing\n");
	}
}
