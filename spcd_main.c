#include "spcd.h"
#include "version.h"

MODULE_LICENSE("GPL");

#define NUM_FAULTS_DEFAULT 9
#define NUM_MAX_THREADS_DEFAULT 1024
#define SPCD_SHIFT_DEFAULT 12
#define SPCD_MEM_HASH_BITS_DEFAULT 22

static struct task_struct *pf_thread;
static struct task_struct *map_thread;

int num_faults = NUM_FAULTS_DEFAULT;
int do_map = 0;
int max_threads = NUM_MAX_THREADS_DEFAULT;
int max_threads_bits = 0;
int spcd_shift = SPCD_SHIFT_DEFAULT;
int spcd_mem_hash_bits = SPCD_MEM_HASH_BITS_DEFAULT;
module_param(num_faults, int, 0);
module_param(do_map, int, 0);
module_param(max_threads, int, 0);
module_param(spcd_shift, int, 0);
module_param(spcd_mem_hash_bits, int, 0);

struct spcd_share_matrix spcd_main_matrix;

int init_module(void)
{
	max_threads = roundup_pow_of_two(max_threads);
	max_threads_bits = ilog2(max_threads);

	printk("SPCD: Start (version %s)\n", SPCD_VERSION);
	printk("    num_faults: %d %s\n", num_faults, num_faults==NUM_FAULTS_DEFAULT ? "(default)" : "");
	printk("    max_threads: %d %s\n", max_threads, max_threads==NUM_MAX_THREADS_DEFAULT ? "(default)" : "");
	printk("    use mapping: %s\n", do_map ? "yes" : "no");
	printk("    shift: %d bits %s\n", spcd_shift, spcd_shift==SPCD_SHIFT_DEFAULT ? "(default)" : "");
	printk("    mem size: %d bits, %d elements %s\n", spcd_mem_hash_bits, 1<<spcd_mem_hash_bits, spcd_mem_hash_bits==SPCD_MEM_HASH_BITS_DEFAULT ? 
		"(default)" : "");

	spcd_main_matrix.matrix = NULL;
	spin_lock_init(&spcd_main_matrix.lock);

	reset_stats();
	register_probes();

	spcd_proc_init();

	// pf_thread = kthread_create(spcd_pagefault_func, NULL, "spcd_pf_thread");
	// wake_up_process(pf_thread);

	if (do_map) {
		map_thread = kthread_create(spcd_map_func, NULL, "spcd_map_thread");
		wake_up_process(map_thread);
	}

	//interceptor_start();
	topo_start();

	return 0;
}


void cleanup_module(void)
{
	if (pf_thread)
		kthread_stop(pf_thread);

	if (map_thread)
		kthread_stop(map_thread);

	if (spcd_main_matrix.matrix)
		kfree(spcd_main_matrix.matrix);

	spcd_proc_cleanup();

	unregister_probes();

	//interceptor_stop();
	topo_stop();
	pt_mem_stop();

	printk("SPCD: Quit (version %s)\n", SPCD_VERSION);
}
