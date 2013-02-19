#include "spcd.h"
#include "version.h"

MODULE_LICENSE("GPL");

#define NUM_FAULTS_DEFAULT 3
#define NUM_MAX_THREADS_DEFAULT 32 

static struct task_struct *pf_thread;
static struct task_struct *map_thread;

unsigned **share;

int num_faults = NUM_FAULTS_DEFAULT;
int do_map = 0;
int max_threads = NUM_MAX_THREADS_DEFAULT;
module_param(num_faults, int, 0);
module_param(do_map, int, 0);
module_param(max_threads, int, 0);

int init_module(void)
{
	int i;

	printk("SPCD: Start (version %s)\n", SPCD_VERSION);
	printk("    num_faults: %d %s\n", num_faults, num_faults==NUM_FAULTS_DEFAULT ? "(default)" : "");
	printk("    max_threads: %d\n", max_threads);
	printk("    use mapping: %s\n", do_map ? "yes" : "no");


	share = (unsigned **) kmalloc (max_threads * sizeof(unsigned *), GFP_KERNEL);
	for (i=0; i<max_threads; i++)
		share[i] = (unsigned *) kmalloc (max_threads * sizeof(unsigned), GFP_KERNEL);

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
	int i;

	if (pf_thread)
		kthread_stop(pf_thread);

	if (map_thread)
		kthread_stop(map_thread);

	for (i=0; i<max_threads; i++) {
		if (share[i])
			kfree(share[i]);
	}
	if (share)
		kfree(share);

	unregister_probes();
	interceptor_end();

	topo_cleanup();

	printk("SPCD: Quit (version %s)\n", SPCD_VERSION);
}
