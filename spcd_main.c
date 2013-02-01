#include "spcd.h"

MODULE_LICENSE("GPL");

#define NUM_FAULTS_DEFAULT 3

static struct task_struct *pf_thread;
// static struct task_struct *map_thread;

int num_faults = NUM_FAULTS_DEFAULT;
module_param(num_faults, int, 0);

int init_module(void)
{
	printk("SPCD: Welcome.....\n");
	printk("SPCD: num_faults: %d %s\n", num_faults, num_faults==NUM_FAULTS_DEFAULT ? "(default)" : "");
	printk("SPCD: max_threads: %d\n", PT_MAXTHREADS);


	reset_stats();
	register_probes();

	pf_thread = kthread_create(spcd_pagefault_func, NULL, "spcd_pf_thread");
	wake_up_process(pf_thread);

	// map_thread = kthread_create(spcd_map_func, NULL, "spcd_map_thread");
	// wake_up_process(map_thread);

	interceptor_start();

	// topo_init();

	return 0;
}


void cleanup_module(void)
{
	if (pf_thread)
		kthread_stop(pf_thread);

	// if (map_thread)
	// 	kthread_stop(map_thread);

	unregister_probes();
	interceptor_end();

	// topo_cleanup();

	printk("Bye.....\n");
}
