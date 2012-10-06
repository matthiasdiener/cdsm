#include "pt_comm.h"

MODULE_LICENSE("GPL");

struct task_struct *pt_task;
unsigned long pt_pf;
unsigned long pt_addr_conflict;
unsigned long pt_pf_extra;
unsigned long pt_num_walks ;
int pt_nt = 0;

unsigned pt_num_faults = 3;

unsigned long share [PT_MAXTHREADS][PT_MAXTHREADS];

struct task_struct *pt_thr;

#include "pt_pf_thread.c"
#include "pt_pid.c"
#include "pt_mem.c"


int init_module(void)
{
	printk("Welcome.....\n");
	pt_reset_all();
	pt_thr = kthread_create(pt_pf_func, NULL, "pt_pf_func");
	wake_up_process(pt_thr);
	return 0;
}


void cleanup_module(void)
{
	kthread_stop(pt_thr);
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



void pt_check_comm(struct task_struct *tsk, unsigned long address)
{
	DEFINE_SPINLOCK(ptl);
	int mytid;
	struct pt_mem_info *elem;

	// detection not requested -> return
	if (pt_task == 0)
		return;
	
	mytid = pt_get_tid(tsk->pid);
	// thread not in list -> we are not interested
	if (mytid == -1)
		return;


	elem = pt_get_mem(address);

	spin_lock(&ptl);

	if (elem->pg_addr != (address >> PAGE_SHIFT) ){
		if (elem->pg_addr !=0 ) {
			// printk ("XXX conflict, hash = %ld, old = %lu, new = %lu\n", h, elem->pg_addr, (address >> PAGE_SHIFT));
			pt_addr_conflict++;
		}
		elem->sharer[0] = -1;
		elem->sharer[1] = -1;
		elem->pg_addr = address >> PAGE_SHIFT;
	}

	// no sharer present
	if (elem->sharer[0] == -1 && elem->sharer[1] == -1) {
		elem->sharer[0] = mytid;
		goto out;
	}

	// two sharers present
	if (elem->sharer[0] != -1 && elem->sharer[1] != -1) {

		// both different from myself
		if (elem->sharer[0] != mytid && elem->sharer[1] != mytid) {
			share[mytid][elem->sharer[0]] ++;
			share[mytid][elem->sharer[1]] ++;
			
			elem->sharer[1] = elem->sharer[0];
			elem->sharer[0] = mytid;

			goto out;
		}


		if (elem->sharer[0] == mytid) {
			share[mytid][elem->sharer[1]] ++;
			goto out;
		}

		if (elem->sharer[1] == mytid) {
			share[mytid][elem->sharer[0]] ++;
			goto out;
		}


		goto out;
	}

	// only second sharer present
	if (elem->sharer[0] == -1) {
		if (elem->sharer[1] != mytid) {
			elem->sharer[0] = mytid;
			share[mytid][elem->sharer[1]] ++;
		}
		goto out;
	}

	// only first sharer present
	if (elem->sharer[0] != mytid) {
		elem->sharer[1] = mytid;
		share[mytid][elem->sharer[0]] ++;
	}

	out:
		spin_unlock(&ptl);

}


void pt_print_comm(void)
{
	int i,j;
	int nt = pt_get_numthreads();
	for (i = nt-1; i >= 0; i--) {
		for (j = 0; j < nt; j++){
			printk ("%lu", share[i][j]+share[j][i]);
			if (j != nt-1)
				printk (",");
		}
		printk("\n");
	}
}

