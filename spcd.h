#ifndef __PT_COMM_H
#define __PT_COMM_H

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/hash.h>
#include <linux/kthread.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>
#include <asm-generic/tlb.h>
#include <linux/slab.h>

#define TSC_DELTA 100000*1000*1000UL

#define PT_PID_HASH_BITS 14UL
#define PT_PID_HASH_SIZE (1UL << PT_PID_HASH_BITS)

struct pt_mem_info {
	unsigned long pg_addr;
	unsigned long tsc;
	s16 sharer[2];
};

extern int max_threads;
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

extern void reset_stats(void);

/* PID/TID functions */
int pt_get_tid(int pid); 
void pt_add_pid(int pid);
void pt_delete_pid(int pid);
void pt_pid_clear(void);
int spcd_get_num_threads(void);
int spcd_get_active_threads(void);

/* Mem functions */
struct pt_mem_info* pt_get_mem_init(unsigned long addr);
void pt_mem_clear(void);

/* Communication functions */
void pt_check_comm(int tid, unsigned long address);
void pt_print_share(void);
void pt_share_clear(void);
unsigned get_share(int i, int j);

/* PF thread */
int spcd_pagefault_func(void* v);
void spcd_pf_thread_clear(void);

/* Map thread */
int spcd_map_func(void* v);

/* Utility */
void register_probes(void);
void unregister_probes(void);

/* Intercept */
int interceptor_start(void);
void interceptor_end(void);

/* Topology */
void topo_init(void);
void topo_cleanup(void);

#endif
