#include "pt_comm.h"

static struct pt_mem_info pt_mem[PT_MEM_HASH_SIZE];


struct pt_mem_info* pt_get_mem(unsigned long addr)
{
	unsigned long h = hash_32(addr >> PAGE_SHIFT, PT_MEM_HASH_BITS);
	return &pt_mem[h];
}

void pt_mark_pte(unsigned long address)
{
	struct pt_mem_info *elem = pt_get_mem(address);

	if (elem->pg_addr != (address >> PAGE_SHIFT) ){
		if (elem->pg_addr !=0 ) {
			if (elem->pte_cleared){
				pt_fix_pte(address);
			}
			printk ("XXX conflict, hash = %u, old = %lu, new = %lu\n", hash_32(address >> PAGE_SHIFT, PT_MEM_HASH_BITS), elem->pg_addr, (address >> PAGE_SHIFT));
			pt_addr_conflict++;
		}
		elem->sharer[0] = -1;
		elem->sharer[1] = -1;
		elem->pg_addr = address >> PAGE_SHIFT;
		elem->pte_cleared = 0;
	}

	if (elem->pte_cleared){
		pt_fix_pte(address);
	}

	elem->pte_cleared = 1;
}

void pt_fix_pte(unsigned long addr)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	spinlock_t *ptl;

	pgd = pgd_offset(pt_task->mm, addr);
	pud = pud_offset(pgd, addr);
	pmd = pmd_offset(pud, addr);
	pte = pte_offset_map_lock(pt_task->mm, pmd, addr, &ptl);
	*pte = pte_set_flags(*pte, _PAGE_PRESENT);
	pt_pte_fixes ++;
	pte_unmap_unlock(pte, ptl);
}

void pt_mem_clear(void)
{
	memset(pt_mem, 0, sizeof(pt_mem));
}