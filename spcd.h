#ifndef __SPCD_COMM_H
#define __SPCD_COMM_H

#include <linux/delay.h>
#include <linux/hash.h>
#include <linux/kthread.h>
#include <linux/kallsyms.h>
#include <asm-generic/tlb.h>

#include <linux/version.h>

#include "libmapping.h"

#define TSC_DELTA 100000*1000*1000UL

#define SPCD_PID_HASH_BITS 14UL
#define SPCD_PID_HASH_SIZE (1UL << SPCD_PID_HASH_BITS)

struct spcd_mem_info {
	unsigned long pg_addr;
	unsigned long tsc;
	s16 sharer[2];
};

extern int max_threads;
extern int max_threads_bits;
extern int spcd_shift;
extern int spcd_mem_hash_bits;

extern int do_pf;

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
void spcd_mem_init(void);
void spcd_mem_cleanup(void);

/* Communication functions */
void spcd_check_comm(int tid, unsigned long address);
void spcd_print_comm(void);
void spcd_comm_init(void);

/* PF thread */
int spcd_pagefault_func(void* v);
void spcd_pf_thread_init(void);

/* Mapping */
int spcd_map_func(void* v);
void spcd_set_affinity(int tid, int core);
void spcd_map_init(void);

/* Probes */
void spcd_probes_init(void);
void spcd_probes_cleanup(void);

/* Intercept */
int interceptor_start(void);
void interceptor_stop(void);

/* Topology */
void topo_init(void);
extern int num_nodes, num_cpus, num_cores, num_threads, pu[256];

/* Communication Matrix */
extern struct spcd_comm_matrix spcd_matrix;

static inline
unsigned get_comm(int first, int second)
{
	if (first > second)
		return spcd_matrix.matrix[(first << max_threads_bits) + second];
	else
		return spcd_matrix.matrix[(second << max_threads_bits) + first];
}


static inline
void inc_comm(int first, int second, unsigned old_tsc, unsigned long new_tsc)
{
	/* TODO:
		- replace with Atomic incr.
		- check if spinlock needed
		- compare TSC_DELTA
	*/

	if (first > second)
		spcd_matrix.matrix[(first << max_threads_bits) + second] ++;
	else
		spcd_matrix.matrix[(second << max_threads_bits) + first] ++;
}

/* ProcFS */
int spcd_proc_init (void);
void spcd_proc_cleanup(void);


#endif
