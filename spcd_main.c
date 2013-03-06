#include "spcd.h"
#include "version.h"

MODULE_LICENSE("GPL");

#define NUM_FAULTS_DEFAULT 3
#define NUM_MAX_THREADS_DEFAULT 32 
#define SPCD_SHIFT_DEFAULT 12
#define SPCD_MEM_HASH_BITS_DEFAULT 22

static struct task_struct *pf_thread;
static struct task_struct *map_thread;

int num_faults = NUM_FAULTS_DEFAULT;
int do_map = 0;
int max_threads = NUM_MAX_THREADS_DEFAULT;
int spcd_shift = SPCD_SHIFT_DEFAULT;
int spcd_mem_hash_bits = SPCD_MEM_HASH_BITS_DEFAULT;
module_param(num_faults, int, 0);
module_param(do_map, int, 0);
module_param(max_threads, int, 0);
module_param(spcd_shift, int, 0);
module_param(spcd_mem_hash_bits, int, 0);


int init_module(void)
{
	printk("SPCD: Start (version %s)\n", SPCD_VERSION);
	printk("    num_faults: %d %s\n", num_faults, num_faults==NUM_FAULTS_DEFAULT ? "(default)" : "");
	printk("    max_threads: %d\n", max_threads);
	printk("    use mapping: %s\n", do_map ? "yes" : "no");
	printk("    shift (in bits): %d %s\n", spcd_shift, spcd_shift==SPCD_SHIFT_DEFAULT ? "(default)" : "");
	printk("    mem size: %d bits, %d elements\n", spcd_mem_hash_bits, 1<<spcd_mem_hash_bits);

	reset_stats();
	register_probes();

	pf_thread = kthread_create(spcd_pagefault_func, NULL, "spcd_pf_thread");
	wake_up_process(pf_thread);

	if (do_map) {
		map_thread = kthread_create(spcd_map_func, NULL, "spcd_map_thread");
		wake_up_process(map_thread);
	}

	interceptor_start();
	topo_start();

	return 0;
}


void cleanup_module(void)
{
	if (pf_thread)
		kthread_stop(pf_thread);

	if (map_thread)
		kthread_stop(map_thread);

	if (share)
		kfree(share);

	unregister_probes();

	interceptor_stop();
	topo_stop();

	printk("SPCD: Quit (version %s)\n", SPCD_VERSION);
}
