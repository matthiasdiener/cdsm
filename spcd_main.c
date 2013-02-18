#include "spcd.h"
#include "version.h"

MODULE_LICENSE("GPL");

#define NUM_FAULTS_DEFAULT 3

static struct task_struct *pf_thread;
static struct task_struct *map_thread;

int num_faults = NUM_FAULTS_DEFAULT;
int do_map = 0;
module_param(num_faults, int, 0);
module_param(do_map, int, 0);

int init_module(void)
{
	printk("SPCD: Start (version %s)\n", SPCD_VERSION);
	printk("\tnum_faults: %d %s\n", num_faults, num_faults==NUM_FAULTS_DEFAULT ? "(default)" : "");
	printk("\tmax_threads: %d\n", PT_MAXTHREADS);
	printk("\tuse mapping: %s\n", do_map ? "yes" : "no");


	reset_stats();
	register_probes();

	pf_thread = kthread_create(spcd_pagefault_func, NULL, "spcd_pf_thread");
	wake_up_process(pf_thread);

	if (do_map) {
		map_thread = kthread_create(spcd_map_func, NULL, "spcd_map_thread");
		wake_up_process(map_thread);
	}

	interceptor_start();

	topo_init();

	return 0;
}


void cleanup_module(void)
{
	if (pf_thread)
		kthread_stop(pf_thread);

	if (map_thread)
		kthread_stop(map_thread);

	unregister_probes();
	interceptor_end();

	topo_cleanup();

	printk("Bye.....\n");
}
