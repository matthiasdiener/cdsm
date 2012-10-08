#include "pt_comm.h"

MODULE_LICENSE("GPL");

struct task_struct *pt_task;
unsigned long pt_pf = 0;
unsigned long pt_addr_conflict;
unsigned long pt_pf_extra = 0;
unsigned pt_num_faults;
int pt_nt = 0;

unsigned pt_num_faults = 3;

unsigned long share [PT_MAXTHREADS][PT_MAXTHREADS];

struct task_struct *pt_thr;

static int (*spcd_func_original_ref)(struct task_struct *, unsigned long); 
extern int (*spcd_func)(struct task_struct *, unsigned long);

static void (*spcd_new_process_original_ref)(struct task_struct *); 
extern void (*spcd_new_process)(struct task_struct *);

#include "pagewalk.c"
#include "pt_pf_thread.c"
#include "pt_pid.c"
#include "pt_mem.c"
#include "pt_dpf.c"


void spcd_new_process_new(struct task_struct *task)
{
	printk("created process %s, %d\n" task->comm, task->pid);
	char name[] = ".x";
	if (strstr(task->comm, name)) {
		if (pt_task == 0) {
			printk("pt: start %s (pid %d) \n", task->comm, task->pid);
			pt_task = task;
		}
		pt_add_pid(task->pid, pt_nt++);
	}
	
}


int spcd_func_new(struct task_struct *tsk, unsigned long address)
{
	int tid = pt_get_tid(tsk->pid);
	if (tid > -1) {
		pt_pf++;
		pt_check_comm(tid, address);
	}
	return 0;
}


int init_module(void)
{
	printk("Welcome.....\n");
	pt_reset_all();
	spcd_func_original_ref = spcd_func;
	spcd_func = &spcd_func_new;

	spcd_new_process_original_ref = spcd_new_process;
	spcd_new_process = &spcd_new_process_new;

	pt_thr = kthread_create(pt_pf_func, NULL, "pt_pf_func");
	wake_up_process(pt_thr);
	return 0;
}


void cleanup_module(void)
{
	kthread_stop(pt_thr);
	spcd_func = spcd_func_original_ref;
	spcd_new_process = spcd_new_process_original_ref;
	printk("Bye.....\n");
}


void pt_reset_all(void)
{
	pt_reset();
	pt_reset_stats();
}


void pt_reset(void)
{
	pt_task = 0;	
}


void pt_reset_stats(void)
{
	pt_pid_clear();
	pt_nt = 0;
	pt_num_walks = 0;
	pt_pf = 0;
	pt_pf_extra = 0;
	pt_addr_conflict = 0;
	memset(share, 0, sizeof(share));
}



void pt_print_stats(void)
{
	int i,j;
	int nt = pt_get_numthreads();

	printk("(%d threads): %lu pfs (%lu extra), %lu walks\n", nt, pt_pf, pt_pf_extra, pt_num_walks);

	for (i = nt-1; i >= 0; i--) {
		for (j = 0; j < nt; j++){
			printk ("%lu", share[i][j]+share[j][i]);
			if (j != nt-1)
				printk (",");
		}
		printk("\n");
	}
}
