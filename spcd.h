#ifndef __PT_COMM_H
#define __PT_COMM_H

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

#define TSC_DELTA 100000*1000*1000UL

#define PT_PID_HASH_BITS 14UL
#define PT_PID_HASH_SIZE (1UL << PT_PID_HASH_BITS)

struct pt_mem_info {
	unsigned long pg_addr;
	unsigned long tsc;
	s16 sharer[2];
};

struct spcd_share_matrix {
	unsigned *matrix;
	spinlock_t lock;
};

extern int max_threads;
extern int max_threads_bits;
extern int spcd_shift;
extern unsigned *share;
extern int spcd_mem_hash_bits;

extern unsigned long pt_pte_fixes;
extern unsigned long pt_pf;
extern unsigned long pt_addr_conflict;
extern unsigned long pt_pf_extra;
extern struct task_struct *pt_task;
extern struct mm_struct *pt_mm;

extern unsigned long pt_num_walks;

void reset_stats(void);

/* PID/TID functions */
int pt_get_tid(int pid); 
int pt_get_pid(int tid);
void pt_add_pid(int pid);
void pt_delete_pid(int pid);
void pt_pid_clear(void);
int spcd_get_num_threads(void);
int spcd_get_active_threads(void);

/* Mem functions */
struct pt_mem_info* pt_get_mem_init(unsigned long addr);
void pt_mem_clear(void);
void pt_mem_stop(void);

/* Communication functions */
void pt_check_comm(int tid, unsigned long address);
void pt_print_share(void);
void pt_share_clear(void);

/* PF thread */
int spcd_pagefault_func(void* v);
void spcd_pf_thread_clear(void);

/* Map thread */
int spcd_map_func(void* v);

/* Probes */
void register_probes(void);
void unregister_probes(void);

/* Intercept */
int interceptor_start(void);
void interceptor_stop(void);

/* Topology */
void topo_start(void);
void topo_stop(void);
extern int num_nodes, num_cores, num_threads;

/* Share Matrix */
extern struct spcd_share_matrix spcd_main_matrix;

static inline
unsigned get_share(int i, int j)
{
	int res;
	
	spin_lock(&spcd_main_matrix.lock);
	res = i > j ? spcd_main_matrix.matrix[(i<<max_threads_bits) + j] : spcd_main_matrix.matrix[(j<<max_threads_bits) + i];
	spin_unlock(&spcd_main_matrix.lock);
	
	return res;
}

/* ProcFS */
int spcd_proc_init (void);
void spcd_proc_cleanup(void);


#endif
