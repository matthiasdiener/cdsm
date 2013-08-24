#ifndef __SPCD_COMM_H
#define __SPCD_COMM_H

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/hash.h>
#include <linux/kthread.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>
#include <linux/spinlock.h>
#include <asm-generic/tlb.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/version.h>

#include "libmapping.h"

#define TSC_DELTA 100000*1000*1000UL

#define SPCD_PID_HASH_BITS 14UL
#define SPCD_PID_HASH_SIZE (1UL << SPCD_PID_HASH_BITS)

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,8,0)
	#define ENABLE_EXTRA_PF 1
	#pragma message "ENABLE_EXTRA_PF"
#endif

struct spcd_mem_info {
	unsigned long pg_addr;
	unsigned long tsc;
	s16 sharer[2];
};

extern int max_threads;
extern int max_threads_bits;
extern int spcd_shift;
extern int spcd_mem_hash_bits;

extern unsigned long spcd_pte_fixes;
extern unsigned long spcd_pf;
extern unsigned long spcd_addr_conflict;
extern unsigned long spcd_pf_extra;
extern int spcd_vma_shared_flag;

void reset_stats(void);

/* PID/TID functions */
int spcd_get_tid(int pid);
int spcd_get_pid(int tid);
int spcd_add_pid(int pid);
void spcd_delete_pid(int pid);
void spcd_pid_clear(void);
int spcd_get_num_threads(void);
int spcd_get_active_threads(void);

/* Mem functions */
struct spcd_mem_info* spcd_get_mem_init(unsigned long addr);
void spcd_mem_clear(void);
void spcd_mem_stop(void);

/* Communication functions */
void spcd_check_comm(int tid, unsigned long address);
void spcd_print_share(void);
void spcd_share_clear(void);

/* PF thread */
int spcd_pagefault_func(void* v);
void spcd_pf_thread_clear(void);

/* Mapping */
int spcd_map_func(void* v);
void spcd_set_affinity(int tid, int core);
void spcd_map_init(void);

/* Probes */
void register_probes(void);
void unregister_probes(void);

/* Intercept */
int interceptor_start(void);
void interceptor_stop(void);

/* Topology */
void topo_start(void);
void topo_stop(void);
extern int num_nodes, num_cpus, num_cores, num_threads, pu[256];

/* Share Matrix */
extern struct spcd_share_matrix spcd_main_matrix;

static inline
unsigned get_share(int i, int j)
{
	int res;

	res = i > j ? spcd_main_matrix.matrix[(i<<max_threads_bits) + j] : spcd_main_matrix.matrix[(j<<max_threads_bits) + i];

	return res;
}


static inline
void inc_share(int first, int second, unsigned old_tsc, unsigned long new_tsc)
{
	// TODO: replace with Atomic incr.

	// spin_lock(&spcd_main_matrix.lock);
	// if (new_tsc-old_tsc <= TSC_DELTA) {
	if (first > second)
		spcd_main_matrix.matrix[(first << max_threads_bits) + second] ++;
	else
		spcd_main_matrix.matrix[(second << max_threads_bits) + first] ++;
	// }
	// spin_unlock(&spcd_main_matrix.lock);
}

/* ProcFS */
int spcd_proc_init (void);
void spcd_proc_cleanup(void);


#endif
