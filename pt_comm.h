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

struct pt_mem_info {
	unsigned long pg_addr;
	unsigned sharer[2];
};

extern int pt_do;

int pt_get_tid(int pid);
int pt_get_numthreads(void);
int pt_add_pid(int pid, int tid);
void pt_pid_clear(void);

struct pt_mem_info* pt_get_mem(unsigned long addr);
void pt_mem_clear(void);

void pt_print_stats(void);

void pt_reset_all(void);
void pt_reset(void);
void pt_reset_stats(void);

void pt_check_comm(int tid, unsigned long address);

extern void (*do_page_fault_original)(struct pt_regs *, unsigned long);

#endif