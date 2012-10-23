#include "pt_comm.h"

static struct pt_mem_info pt_mem[PT_MEM_HASH_SIZE];


static inline unsigned long addr_to_page(unsigned long address)
{
	return (address >> PAGE_SHIFT);
}


struct pt_mem_info* pt_get_mem(unsigned long address)
{
	unsigned long h = hash_32(addr_to_page(address), PT_MEM_HASH_BITS);
	return &pt_mem[h];
}


/* get mem elem, initialize if necessary */
struct pt_mem_info* pt_get_mem_init(unsigned long address)
{
	unsigned long h = hash_32(addr_to_page(address), PT_MEM_HASH_BITS);
	struct pt_mem_info *elem = &pt_mem[h];
	unsigned long page = addr_to_page(address);

	if (elem->pg_addr != page) { /* new elem */
		if (elem->pg_addr != 0) { /* delete old elem */
			if (elem->pte_cleared)
				pt_fix_pte(elem, address);

			printk ("XXX conflict, hash = %lu, old = %lu, new = %lu\n", h, elem->pg_addr, page);
			pt_addr_conflict++;
		}

		elem->sharer[0] = -1;
		elem->sharer[1] = -1;
		elem->pg_addr = page;
		elem->pte_cleared = 0;
	}

	return elem;
}


/*mark page as present bit cleared by page walk */
void pt_mark_pte(unsigned long address)
{
	
	struct pt_mem_info *elem = pt_get_mem_init(address);
	elem->pte_cleared = 1;
	pt_pf_extra++;
}


/* undo articifical clearing of present bit */
void pt_fix_pte(struct pt_mem_info *elem, unsigned long address)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	spinlock_t *ptl;

	elem->pte_cleared = 0;

	pgd = pgd_offset(pt_task->mm, address);
	pud = pud_offset(pgd, address);
	pmd = pmd_offset(pud, address);
	pte = pte_offset_map_lock(pt_task->mm, pmd, address, &ptl);
	*pte = pte_set_flags(*pte, _PAGE_PRESENT);

	pte_unmap_unlock(pte, ptl);

	pt_pte_fixes++;
}


void pt_mem_clear(void)
{
	memset(pt_mem, 0, sizeof(pt_mem));
}
