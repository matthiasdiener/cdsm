#include "spcd.h"

static int factor_walk = 61;
static struct vm_area_struct *pt_next_vma = NULL;
static unsigned long pt_next_addr = 0;
static unsigned pt_num_faults = 3;

static void pt_pf_pagewalk(struct mm_struct *mm);

static struct page* (*vm_normal_page_p)(struct vm_area_struct *vma, unsigned long addr, pte_t pte) = NULL;

static int (*walk_page_range_p)(unsigned long addr, unsigned long end,
			struct mm_walk *walk) = NULL;

static pid_t (*vm_is_stack_p)(struct task_struct *task,
							struct vm_area_struct *vma, int in_group) = NULL;


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


static inline unsigned long vma_size(struct vm_area_struct *vma)
{
	return vma->vm_end - vma->vm_start;
}

static inline int is_heap(struct mm_struct *mm, struct vm_area_struct *vma)
{
	return (vma->vm_start <= mm->brk && vma->vm_end >= mm->start_brk);
}

static inline int is_writable(struct vm_area_struct *vma)
{
	return vma->vm_flags & VM_WRITE ? 1 : 0;
}

static inline int is_vdso(struct vm_area_struct *vma)
{
	return (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso);
}

static inline int is_file(struct vm_area_struct *vma)
{
	return vma->vm_file ? 1 : 0;
}

static inline int is_stack(struct mm_struct *mm, struct vm_area_struct* vma)
{
	return (*vm_is_stack_p)(mm->owner, vma, 1) ? 1 : 0;
}


static inline struct vm_area_struct *find_good_vma(struct mm_struct *mm, struct vm_area_struct* prev_vma)
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

		// printk("tmp: %p, size: %10lu, writeable: %d, is_vdso: %d, is_file: %d, is_heap: %d, is_stack: %d\n", tmp, vma_size(tmp), is_writable(tmp), is_vdso(tmp), is_file(tmp), is_heap(mm, tmp), is_stack(mm, tmp));

		if (is_vdso(tmp) || vma_size(tmp) <= 8096 || is_file(tmp) || is_stack(mm, tmp))
			continue;
		
		if (is_writable(tmp) || is_heap(mm, tmp))
			return tmp;
	}
}


static inline void find_next_vma(struct mm_struct *mm, struct vm_area_struct* prev_vma)
{
	pt_next_vma = find_good_vma(mm, prev_vma);
	pt_next_addr = pt_next_vma->vm_start;
	// factor_walk = vma_size(pt_next_vma) / 1024;
	// printk ("Size:%lu, Factor: %d\n", vma_size(pt_next_vma), factor_walk);
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
	
	down_read(&mm->mmap_sem);

	for (i = 0; i < pt_num_faults; i++) {

		pt_addr_pbit_changed = 0;

		if (pt_next_vma == NULL) {
			find_next_vma(mm, NULL);
		}
		
		while (pt_addr_pbit_changed == 0) {

			pt_addr_pbit_changed = (*walk_page_range_p)(pt_next_addr, pt_next_vma->vm_end, &walk);
			
			if (pt_addr_pbit_changed) {
				pt_next_addr += PAGE_SIZE*((get_cycles()%factor_walk+1) + 1); //Magic
				if (pt_next_addr >= pt_next_vma->vm_end) {
					find_next_vma(mm, pt_next_vma);
				}
			}
			else {
				find_next_vma(mm, pt_next_vma);
			}
		}
	}

	up_read(&mm->mmap_sem);
}


inline void spcd_pf_thread_clear(void)
{
	factor_walk = 61;
	pt_num_walks = 0;
	pt_next_addr = 0;
	pt_next_vma = NULL;
	pt_num_faults = 3;
	pt_pf_extra = 0;
	if (!walk_page_range_p) {
		vm_normal_page_p = (void*) kallsyms_lookup_name("vm_normal_page");
		vm_is_stack_p = (void*) kallsyms_lookup_name("vm_is_stack");
		walk_page_range_p = (void*) kallsyms_lookup_name("walk_page_range");
	}
}
