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

static struct task_struct *spcd_pf_thread;
static struct task_struct *spcd_map_thread;

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
module_param(do_pf, int, 0);

struct spcd_comm_matrix spcd_matrix = {.matrix = NULL, .nthreads = 0};

int spcd_num_faults;

int init_module(void)
{
	max_threads = roundup_pow_of_two(max_threads);
	max_threads_bits = ilog2(max_threads);
	spcd_num_faults = num_faults;

	printk("SPCD: Start (version %s)\n", SPCD_VERSION);
	printk("    additional pagefaults (do_pf): %s %s\n", do_pf ? "yes" : "no", do_pf==ENABLE_EXTRA_PF ? "(default)" : "");
	printk("    extra pagefaults (num_faults): %d %s\n", num_faults, num_faults==NUM_FAULTS_DEFAULT ? "(default)" : "");
	printk("    maximum threads (max_threads): %d %s\n", max_threads, max_threads==NUM_MAX_THREADS_DEFAULT ? "(default)" : "");
	printk("    use mapping (do_map): %s\n", do_map ? "yes" : "no (default)");
	printk("    granularity (spcd_shift): %d bits %s\n", spcd_shift, spcd_shift==SPCD_SHIFT_DEFAULT ? "(default)" : "");
	printk("    mem hash table size (spcd_mem_hash_bits): %d bits, %d elements %s\n", spcd_mem_hash_bits, 1<<spcd_mem_hash_bits, spcd_mem_hash_bits==SPCD_MEM_HASH_BITS_DEFAULT ?
		"(default)" : "");

	spin_lock_init(&spcd_matrix.lock);

	reset_stats();
	spcd_probes_init();

	spcd_proc_init();
	spcd_map_init();


	if (do_pf) {
		spcd_pf_thread = kthread_create(spcd_pagefault_func, NULL, "spcd_pf_thread");
		wake_up_process(spcd_pf_thread);
	}

	topo_init();

	if (do_map) {
		spcd_map_thread = kthread_create(spcd_map_func, NULL, "spcd_map_thread");
		wake_up_process(spcd_map_thread);
	}

	return 0;
}


void cleanup_module(void)
{
	if (spcd_pf_thread)
		kthread_stop(spcd_pf_thread);

	if (spcd_map_thread)
		kthread_stop(spcd_map_thread);

	if (spcd_matrix.matrix)
		kfree(spcd_matrix.matrix);

	spcd_proc_cleanup();

	spcd_probes_cleanup();

	spcd_mem_cleanup();

	printk("SPCD: Quit (version %s)\n", SPCD_VERSION);
}
