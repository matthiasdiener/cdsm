#include <linux/syscalls.h>
#include "spcd.h"


unsigned long **sys_call_table;

asmlinkage long (*ref_sys_ni)(void);


static unsigned long **aquire_sys_call_table(void)
{
	unsigned long int offset = PAGE_OFFSET;
	unsigned long **sct;

	while (offset < ULLONG_MAX) {
		sct = (unsigned long **)offset;

		if (sct[__NR_close] == (unsigned long *) sys_close) 
			return sct;

		offset += sizeof(void *);
	}

	return NULL;
}

static void disable_page_protection(void) 
{
	unsigned long value;
	asm volatile("mov %%cr0, %0" : "=r" (value));

	if(!(value & 0x00010000))
		return;

	asm volatile("mov %0, %%cr0" : : "r" (value & ~0x00010000));
}

static void enable_page_protection(void) 
{
	unsigned long value;
	asm volatile("mov %%cr0, %0" : "=r" (value));

	if((value & 0x00010000))
		return;

	asm volatile("mov %0, %%cr0" : : "r" (value | 0x00010000));
}

static int spcd_get_comm_clear(unsigned long *addr, int clear)
{
	if (addr)
		copy_to_user(addr, share, sizeof(unsigned) * max_threads * max_threads);

	if (clear>=1) {
		pt_share_clear();
		if (clear>=2) {
			pt_mem_clear();
			printk("tb cleared\n");
		}

	}
	return spcd_get_num_threads();
}

int interceptor_start(void) 
{
	if(!(sys_call_table = aquire_sys_call_table())) {
		printk("SPCD: can't intercept syscall table\n");
		return -1;
	}

	disable_page_protection();
	ref_sys_ni = (void *)sys_call_table[__NR_get_kernel_syms];
	sys_call_table[__NR_get_kernel_syms] = (unsigned long *)spcd_get_comm_clear;
	enable_page_protection();

	return 0;
}

void interceptor_stop(void) 
{
	if(!sys_call_table)
		return;

	disable_page_protection();
	sys_call_table[__NR_get_kernel_syms] = (unsigned long *)ref_sys_ni;
	enable_page_protection();
}
