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
			printk ("XXX conf, hash: %x, old: %lx, new: %lx\n", hash_32(page, PT_MEM_HASH_BITS), elem->pg_addr, page);
			pt_addr_conflict++;
		}

		elem->sharer[0] = -1;
		elem->sharer[1] = -1;
		elem->pg_addr = page;
	}
	// spin_unlock(&ptl);
	return elem;
}


void pt_mem_clear(void)
{
	memset(pt_mem, 0, sizeof(pt_mem));
}

void pt_share_clear(void)
{
	memset(share, 0, sizeof(share));
}
