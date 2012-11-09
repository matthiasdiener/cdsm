#include "spcd.h"

static int factor_walk = 61;
static struct vm_area_struct *pt_next_vma = NULL;
static unsigned long pt_next_addr = 0;
static unsigned pt_num_faults = 3;

static void pt_pf_pagewalk(struct mm_struct *mm);


int pt_pf_thread_func(void* v)
{
	int nt;
	static int iter = 0;

	while (1) {
		if (kthread_should_stop())
			return 0;
		iter++;

		if (iter % 1000 == 0) {
			//pt_print_share();
			//pt_share_clear();
		}

		nt = spcd_get_active_threads();
		if (nt >= 2) {
			int ratio = pt_pf / (pt_pf_extra + 1);
			if (ratio > 150 && pt_num_faults < 9)
				pt_num_faults++;
			else if (ratio < 100 && pt_num_faults > 1)
				pt_num_faults--;
			//printk ("num: %d, ratio: %d, pf: %lu, extra: %lu\n", pt_num_faults, ratio, pt_pf, pt_pf_extra);
			pt_pf_pagewalk(pt_task->mm);
		}
		msleep(10);
	}

}


static int pt_callback_page_walk(pte_t *pte, unsigned long addr, unsigned long next_addr, struct mm_walk *walk)
{
	struct page *page;
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	spinlock_t *myptl;
	
	if (pte_none(*pte) || !pte_present(*pte) || !pte_young(*pte) || pte_special(*pte))
		return 0;

	page = (*vm_normal_page_p)(pt_next_vma, addr, *pte);
	if (!page || !page->mapping )
		return 0;

	pgd = pgd_offset(walk->mm, addr);
	pud = pud_offset(pgd, addr);
	pmd = pmd_offset(pud, addr);
	pt_next_addr = addr;

	pte = pte_offset_map_lock(walk->mm, pmd, addr, &myptl);
	*pte = pte_clear_flags(*pte, _PAGE_PRESENT);
	pte_unmap_unlock(pte, myptl);
	
	pt_pf_extra++;

	return 1;

}


static inline unsigned long pt_vma_size(struct vm_area_struct *vma)
{
	return vma->vm_end - vma->vm_start;
}


static struct vm_area_struct *pt_find_vma(struct mm_struct *mm, struct vm_area_struct* prev_vma)
{
	struct vm_area_struct *tmp = prev_vma;

	if (!tmp) {
		pt_num_walks++;
		tmp=mm->mmap;
	}
	while (1) {
		tmp=tmp->vm_next;

		if (!tmp) {
			pt_num_walks++;
			tmp=mm->mmap;
		}
		//printk("tmp: %p, size: %lu\n", tmp, pt_vma_size(tmp));

		if ((tmp->vm_mm && tmp->vm_start == (long)tmp->vm_mm->context.vdso)
		    || pt_vma_size(tmp) <= 8096
		    || tmp->vm_file
		   )
			continue;
		

		if ((tmp->vm_flags & VM_WRITE) 
			|| (tmp->vm_start <= mm->brk && tmp->vm_end >= mm->start_brk)
			// || vm_is_stack(mm->owner, tmp, 1) 
			)
		{
			return tmp;
		}
		
	}

}


static void find_next_vma(struct mm_struct *mm, struct vm_area_struct* prev_vma)
{
	pt_next_vma = pt_find_vma(mm, prev_vma);
	pt_next_addr = pt_next_vma->vm_start;
	factor_walk = pt_vma_size(pt_next_vma) / 102;
	printk ("Size:%lu, Factor: %d\n", pt_vma_size(pt_next_vma), factor_walk);
}


static void pt_pf_pagewalk(struct mm_struct *mm)
{
	unsigned pt_addr_pbit_changed = 0;
	int i = 0;

	struct mm_walk walk = {
		// TODO: save pmd here
		.pte_entry = pt_callback_page_walk,
		.mm = mm,
	};

	if (!mm)
		return;
	
	down_write(&mm->mmap_sem);

	// mod_walk = n / 1024;

	for (i=0; i<pt_num_faults; i++) {
		pt_addr_pbit_changed = 0;

		if (pt_next_vma == NULL) {
			find_next_vma(mm, NULL);
		}
		
		while (pt_addr_pbit_changed == 0 && pt_next_vma != NULL) {
			if ((pt_next_vma->vm_flags & VM_WRITE) == 0) {
				pt_next_vma = pt_next_vma->vm_next;
				if (pt_next_vma) {
					pt_next_addr = pt_next_vma->vm_start;
				}
				continue;
			}

			pt_addr_pbit_changed = (*walk_page_range_p)(pt_next_addr, pt_next_vma->vm_end, &walk);
			
			if (pt_addr_pbit_changed) {
				pt_next_addr += PAGE_SIZE*((get_cycles()%factor_walk+1) + 1); //Magic
				if (pt_next_addr >= pt_next_vma->vm_end) {
					pt_next_vma = pt_next_vma->vm_next;
					if (pt_next_vma) {
						pt_next_addr = pt_next_vma->vm_start;
					}
				}
			}
			else {
				pt_next_vma = pt_next_vma->vm_next;
				if (pt_next_vma) {
					pt_next_addr = pt_next_vma->vm_start;
				}
			}
		}
	}

	up_write(&mm->mmap_sem);
}


inline void spcd_pf_thread_clear(void)
{
	factor_walk = 61;
	pt_num_walks = 0;
	pt_next_addr = 0;
	pt_next_vma = NULL;
	pt_num_faults = 3;
	pt_pf_extra = 0;
}
