#include "pt_comm.h"

static struct pt_mem_info pt_mem[PT_MEM_HASH_SIZE];


static inline unsigned long addr_to_page(unsigned long address)
{
	return (address >> PAGE_SHIFT);
}


struct pt_mem_info* pt_get_mem(unsigned long address)
{
	return &pt_mem[hash_32(addr_to_page(address), PT_MEM_HASH_BITS)];
}


/* get mem elem, initialize if necessary */
struct pt_mem_info* pt_get_mem_init(unsigned long address)
{
	struct pt_mem_info *elem;
	unsigned long page = addr_to_page(address);

	// spin_lock(&ptl);
	elem = pt_get_mem(address);

	if (elem->pg_addr != page) { /* new elem */
		if (elem->pg_addr != 0) { /* delete old elem */
			if (elem->pte_cleared)
				pt_fix_pte(elem, address);

			printk ("XXX conf, hash: %x, old: %lx, new: %lx, pte_fixed: %d\n", hash_32(page, PT_MEM_HASH_BITS), elem->pg_addr, page, elem->pte_cleared ? 1 : 0);
			pt_addr_conflict++;
		}

		elem->sharer[0] = -1;
		elem->sharer[1] = -1;
		elem->pg_addr = page;
		elem->pte_cleared = 0;
	}
	// spin_unlock(&ptl);
	return elem;
}


/* mark page as present bit cleared by page walk */
void pt_mark_pte(unsigned long address, pte_t *pte)
{
	struct pt_mem_info *elem = pt_get_mem_init(address);
	elem->pte_cleared = 1;
	pt_pf_extra++;

	//	printk ("clear pte: %08llx , addr: %lx\n", (long long)pte_val(*pte), address);
}


/* undo articifical clearing of present bit */
void pt_fix_pte(struct pt_mem_info *elem, unsigned long address)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	spinlock_t *myptl;

	pgd = pgd_offset(pt_task->mm, address);
	pud = pud_offset(pgd, address);
	pmd = pmd_offset(pud, address);

	pte = pte_offset_map_lock(pt_task->mm, pmd, address, &myptl);
	if (!pte_none(*pte)) {
		*pte = pte_set_flags(*pte, _PAGE_PRESENT);
		elem->pte_cleared = 0; //atomic_t
		pt_pte_fixes++;
	}
	pte_unmap_unlock(pte, myptl);


	//	printk ("rest pte: %08llx , addr: %lx\n", (long long)pte_val(*pte), address);
}


void pt_mem_clear(void)
{
	memset(pt_mem, 0, sizeof(pt_mem));
}
