#include "pt_comm.h"

MODULE_LICENSE("GPL");

struct task_struct *pt_task;
unsigned long pt_pf = 0;
unsigned long pt_addr_conflict;
unsigned long pt_pf_extra = 0;
unsigned long pt_num_walks ;
int pt_nt = 0;

unsigned pt_num_faults = 3;

unsigned long share [PT_MAXTHREADS][PT_MAXTHREADS];

struct task_struct *pt_thr;

// void (*spcd_page_fault)(struct pt_regs *, unsigned long);
static void (*do_page_fault_original_ref)(struct pt_regs *, unsigned long); 
extern void (*my_fault)(struct pt_regs *, unsigned long);

#include "pt_pf_thread.c"
#include "pt_pid.c"
#include "pt_mem.c"
#include "pt_dpf.c"

int init_module(void)
{
	printk("Welcome.....\n");
	pt_reset_all();
	do_page_fault_original_ref = my_fault;
	my_fault = &spcd_page_fault;
	pt_thr = kthread_create(pt_pf_func, NULL, "pt_pf_func");
	wake_up_process(pt_thr);
	return 0;
}


void cleanup_module(void)
{
	kthread_stop(pt_thr);
	my_fault = do_page_fault_original_ref;
	// pt_reset_all();
	printk("Bye....\n");
}


void pt_reset_all(void)
{
	pt_reset();
	pt_reset_stats();
}


void pt_reset(void)
{
	pt_task = 0;
	// pt_mem_clear();
	pt_pid_clear();
}


void pt_reset_stats(void)
{
	pt_nt = 0;
	pt_num_walks = 0;
	pt_pf = 0;
	pt_pf_extra = 0;
	pt_addr_conflict = 0;
	memset(share, 0, sizeof(share));
}



