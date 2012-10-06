#include "pt_comm.h"

static struct pt_mem_info pt_mem[PT_MEM_HASH_SIZE];


struct pt_mem_info* pt_get_mem(unsigned long addr)
{
	unsigned long h = hash_32(addr >> PAGE_SHIFT, PT_MEM_HASH_BITS);
	return &pt_mem[h];
}


void pt_mem_clear(void)
{
	unsigned i;
	for (i=0; i < PT_MEM_HASH_SIZE; i++) {
		pt_mem[i].pg_addr = 0;
	}

}