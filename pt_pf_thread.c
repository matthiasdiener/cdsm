#include "pt_comm.h"

void pt_detect_app(void)
{
	struct task_struct *task, *thread;
	// unsigned long start = get_cycles();
	char name[] = ".x";

	if (pt_task == 0) {
		for_each_process(task)
		{
			if (strstr(task->comm, name)) {
				printk("pt: start %s \n", task->comm);
				pt_task = task;
				thread = task;
				do {
					pt_add_pid(thread->pid, pt_nt++);
				} while_each_thread(task, thread);
			}
		}
	} else if (pid_alive(pt_task)) {
		thread = pt_task;
		do {
			if (pt_add_pid(thread->pid, pt_nt) > -1)
				pt_nt++;
		} while_each_thread(pt_task, thread);

	} else {
		printk("pt: stop\n");
		pt_reset();
		pt_print_stats();
		pt_reset_stats();
	}


	// printk("time taken: %llu\n", get_cycles()-start);
}


int pt_pf_func(void* v)
{
	
	while (1) {
		if (kthread_should_stop())
			return 0;
		pt_detect_app();
		pt_check_next_addr(pt_task->mm);
		msleep(10);
	}
	
		// if (pt_do_detect==1) {
			// pt_check_next_addr(pt_mm_to_detect);
		// }
		
	// }
	return 0;
}

