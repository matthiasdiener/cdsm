#include "spcd.h"

static int factor_walk = 61;
static struct vm_area_struct *pt_next_vma = NULL;
static unsigned long pt_next_addr = 0;
extern int num_faults;

unsigned long pt_pf_extra;
unsigned long pt_num_walks;

static void pt_pf_pagewalk(struct mm_struct *mm);

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
				pt_pf_pagewalk(tsk->mm);

		}
		
	}

}


static inline
int pt_callback_page_walk(pte_t *pte, unsigned long addr, unsigned long next_addr, struct mm_walk *walk)
{
	
	if (pte_none(*pte) || 
		!pte_present(*pte) || 
		!pte_young(*pte) || 
		pte_special(*pte) )
		return 0;

	pt_next_addr = addr;

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



static
struct vm_area_struct *find_good_vma(struct mm_struct *mm, struct vm_area_struct* prev_vma)
{
	struct vm_area_struct *tmp = prev_vma;

	if (!tmp) {
		pt_num_walks++;
		tmp = mm->mmap;
	}

	while (1) {
		tmp = tmp->vm_next;

		if (!tmp) {
			pt_num_walks++;
			tmp = mm->mmap;
		}

		if (is_shared(tmp))
			return tmp;
	}
}


static inline
void find_next_vma(struct mm_struct *mm, struct vm_area_struct* prev_vma)
{
	pt_next_vma = find_good_vma(mm, prev_vma);
	pt_next_addr = pt_next_vma->vm_start;
}


static
void pt_pf_pagewalk(struct mm_struct *mm)
{
	int i;
	struct mm_walk walk = {
		// TODO: save pmd here
		.pte_entry = pt_callback_page_walk,
		.mm = mm,
	};

	if (!mm)
		return;
	
	down_write(&mm->mmap_sem);

	for (i = 0; i < num_faults; i++) {
		unsigned pt_addr_pbit_changed = 0;
		// unsigned long start = pt_num_walks;

		if (spcd_get_active_threads() < 4)
			goto out;

		if (pt_next_vma == NULL) {
			find_next_vma(mm, NULL);
		}
		
		while (pt_addr_pbit_changed == 0) {

			// if ((pt_num_walks-start)>2)
			// 	goto out;

			pt_addr_pbit_changed = (*walk_page_range_p)(pt_next_addr, pt_next_vma->vm_end, &walk);
			
			if (pt_addr_pbit_changed) {
				pt_next_addr += PAGE_SIZE*((get_cycles()%61) + 1); //Magic
				if (pt_next_addr >= pt_next_vma->vm_end) {
					find_next_vma(mm, pt_next_vma);
				}
			}
			else {
				find_next_vma(mm, pt_next_vma);
			}
		}
	}
out:
	up_write(&mm->mmap_sem);
}


void spcd_pf_thread_clear(void)
{
	factor_walk = 61;
	pt_num_walks = 0;
	pt_next_addr = 0;
	pt_next_vma = NULL;
	pt_pf_extra = 0;
	if (!walk_page_range_p) {
		walk_page_range_p = (void*) kallsyms_lookup_name("walk_page_range");
		if (!walk_page_range_p)
			printk("SPCD BUG: walk_page_range missing\n");
	}
}
