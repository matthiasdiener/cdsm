#ifndef __PT_COMM_H
#define __PT_COMM_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/traps.h>
#include <linux/hash.h>
#include <linux/mm.h>
#include <linux/syscalls.h>
#include <linux/kthread.h>

#define PT_MAXTHREADS 64

#define PT_MEM_HASH_BITS 22UL
#define PT_MEM_HASH_SIZE (1UL << PT_MEM_HASH_BITS)

#define PT_PID_HASH_BITS 10UL
#define PT_PID_HASH_SIZE (1ULL << PT_PID_HASH_BITS)

extern unsigned long pt_pf;
extern unsigned long pt_addr_conflict;
extern unsigned long pt_pf_extra;
extern unsigned pt_num_faults;
extern int pt_num_threads;

extern unsigned long pt_pte_fixes;

extern struct task_struct *pt_task;

struct pt_mem_info {
	unsigned long pg_addr;
	int pte_cleared;
	unsigned sharer[2];
};

int pt_get_tid(int pid); 
int pt_add_pid(int pid, int tid);
void pt_pid_clear(void);

struct pt_mem_info* pt_get_mem(unsigned long addr);
void pt_mem_clear(void);

void pt_print_stats(void);

void pt_reset_all(void);
void pt_reset(void);
void pt_reset_stats(void);

void pt_check_comm(int tid, unsigned long address);

void pt_mark_pte(unsigned long addr);
void pt_fix_pte(unsigned long addr);

void pt_pf_pagewalk(struct mm_struct *mm);
int pt_pf_thread_func(void* v);

#endif
