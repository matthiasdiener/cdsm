#include "spcd.h"
#include "version.h"

#include <linux/module.h>

MODULE_LICENSE("GPL");

#define NUM_FAULTS_DEFAULT 9
#define NUM_MAX_THREADS_DEFAULT 1024
#define SPCD_SHIFT_DEFAULT 12
#define SPCD_MEM_HASH_BITS_DEFAULT 22

#ifdef ENABLE_EXTRA_PF
	#define ENABLE_EXTRA_PF 1
#else
	#define ENABLE_EXTRA_PF 0
#endif

static struct task_struct *pf_thread;
static struct task_struct *map_thread;

int num_faults = NUM_FAULTS_DEFAULT;
int do_map = 0;
int max_threads = NUM_MAX_THREADS_DEFAULT;
int max_threads_bits = 0;
int spcd_shift = SPCD_SHIFT_DEFAULT;
int spcd_mem_hash_bits = SPCD_MEM_HASH_BITS_DEFAULT;
int do_pf = ENABLE_EXTRA_PF;
module_param(num_faults, int, 0);
module_param(do_map, int, 0);
module_param(max_threads, int, 0);
module_param(spcd_shift, int, 0);
module_param(spcd_mem_hash_bits, int, 0);
module_param(do_pf, int, 1);

struct spcd_share_matrix spcd_main_matrix = {.matrix = NULL, .nthreads = 0};

int init_module(void)
{
	max_threads = roundup_pow_of_two(max_threads);
	max_threads_bits = ilog2(max_threads);

	printk("SPCD: Start (version %s)\n", SPCD_VERSION);
	printk("    additional pagefaults (do_pf): %s %s\n", do_pf ? "yes" : "no", do_pf==ENABLE_EXTRA_PF ? "(default)" : "");
	printk("    extra pagefaults (num_faults): %d %s\n", num_faults, num_faults==NUM_FAULTS_DEFAULT ? "(default)" : "");
	printk("    maximum threads (max_threads): %d %s\n", max_threads, max_threads==NUM_MAX_THREADS_DEFAULT ? "(default)" : "");
	printk("    use mapping (do_map): %s\n", do_map ? "yes" : "no (default)");
	printk("    granularity (spcd_shift): %d bits %s\n", spcd_shift, spcd_shift==SPCD_SHIFT_DEFAULT ? "(default)" : "");
	printk("    mem hash table size (spcd_mem_hash_bits): %d bits, %d elements %s\n", spcd_mem_hash_bits, 1<<spcd_mem_hash_bits, spcd_mem_hash_bits==SPCD_MEM_HASH_BITS_DEFAULT ?
		"(default)" : "");

	spcd_main_matrix.matrix = NULL;
	spin_lock_init(&spcd_main_matrix.lock);

	reset_stats();
	register_probes();

	spcd_proc_init();
	spcd_map_init();


	if (do_pf) {
		#pragma message "ENABLE_EXTRA_PF"
		pf_thread = kthread_create(spcd_pagefault_func, NULL, "spcd_pf_thread");
		wake_up_process(pf_thread);
	}

	topo_start();

	if (do_map) {
		map_thread = kthread_create(spcd_map_func, NULL, "spcd_map_thread");
		wake_up_process(map_thread);
	}

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

	topo_stop();
	spcd_mem_stop();

	printk("SPCD: Quit (version %s)\n", SPCD_VERSION);
}
