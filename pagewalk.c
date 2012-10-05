static unsigned long pt_next_addr = 0;
static struct vm_area_struct *pt_next_vma = NULL;
static unsigned long num_walks = 3;

static int pt_callback_page_walk(pte_t *pte, unsigned long addr, unsigned long next_addr, struct mm_walk *walk)
{
	struct page *page;
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	spinlock_t *ptl;
	
	if (!pte_present(*pte))
		return 0;

	page = vm_normal_page(pt_next_vma, addr, *pte);
	if (!page)
		return 0;

	if (pte_young(*pte) || PageReferenced(page)) {
		pgd = pgd_offset(walk->mm, addr);
		pud = pud_offset(pgd, addr);
		pmd = pmd_offset(pud, addr);
		pt_next_addr = addr;

		// spin_lock(&walk->mm->page_table_lock);
		pte = pte_offset_map_lock(walk->mm, pmd, addr, &ptl);
		*pte = pte_clear_flags(*pte, _PAGE_PRESENT);
		pte_unmap_unlock(pte, ptl)	;
		// spin_unlock(&walk->mm->page_table_lock);

		pt_extra_pf++;

		return 1;
	}
	
	return 0;
}

static void pt_check_next_addr(struct mm_struct *mm)
{
	unsigned pt_addr_pbit_changed = 0, mod_walk;
	int i = 0;

	struct mm_walk walk = {
		.pte_entry = pt_callback_page_walk,
		.mm = mm,
	};
	
	down_write(&mm->mmap_sem);

	mod_walk = n / 1024;

	for (i=0; i<num_faults; i++) {
		pt_addr_pbit_changed = 0;

		if (pt_next_vma == NULL) {
			num_walks ++;
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

			pt_addr_pbit_changed = walk_page_range(pt_next_addr, pt_next_vma->vm_end, &walk);
			
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

