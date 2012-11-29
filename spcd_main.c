#include "spcd.h"

MODULE_LICENSE("GPL");

unsigned long pt_pf;
unsigned long pt_addr_conflict;
unsigned long pt_pf_extra;
unsigned long pt_num_walks;
unsigned long pt_pte_fixes;

static struct task_struct *pf_thread;
static struct task_struct *map_thread;

int init_module(void)
{
	printk("Welcome.....\n");

	reset_stats();
	register_probes();

	pf_thread = kthread_create(spcd_pagefault_func, NULL, "spcd_pf_thread");
	wake_up_process(pf_thread);

	map_thread = kthread_create(spcd_map_func, NULL, "spcd_map_thread");
	wake_up_process(map_thread);

	return 0;
}


void cleanup_module(void)
{
	kthread_stop(pf_thread);
	kthread_stop(map_thread);

	unregister_probes();

	printk("Bye.....\n");
}
