#include "pt_comm.h"

int pt_callback_page_walk(pte_t *pte, unsigned long addr, unsigned long next_addr, struct mm_walk *walk)
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


void pt_pf_pagewalk(struct mm_struct *mm)
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
			pt_num_walks ++;
			pt_next_vma = mm->mmap;
			pt_next_addr = pt_next_vma->vm_start;
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
				pt_next_addr += PAGE_SIZE*((get_cycles()%61) + 1); //Magic
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
