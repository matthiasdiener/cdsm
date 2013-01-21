#include "spcd.h"

MODULE_LICENSE("GPL");

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

	interceptor_start();

	topo_init();

	return 0;
}


void cleanup_module(void)
{
	kthread_stop(pf_thread);
	kthread_stop(map_thread);

	unregister_probes();
	interceptor_end();

	printk("Bye.....\n");
}
