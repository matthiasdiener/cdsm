#include "pt_comm.h"

void pt_get_children(void)
{
	struct task_struct *task, *thread;
	int nt, tt = 0;
	unsigned long start = get_cycles();

	for_each_process(task)
	{

		if ((nt=get_nr_threads(task))>1) {
			thread = task;
			do {
				// printk ("%s [%d] (%d threads): t: %d\n", task->comm, task->pid, nt, thread->pid);
				tt += nt;
			} while_each_thread(task, thread);
		}
	}

	printk("total threads: %d  time taken: %llu\n", tt, get_cycles()-start);
}


int pt_pf_func(void* v)
{
	
	// while (1) {
	// 	msleep(2000);
		
	pt_get_children();
		// if (pt_do_detect==1) {
			// pt_check_next_addr(pt_mm_to_detect);
		// }
		
	// }
	return 0;
}

