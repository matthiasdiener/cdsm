#include "pt_comm.h"

void pt_detect_app(void)
{
	if (pt_task && !pid_alive(pt_task)) {
		printk("pt: stop\n");
		pt_reset();
		pt_print_stats();
		pt_reset_stats();
	}
}


int pt_pf_func(void* v)
{
	
	while (1) {
		if (kthread_should_stop())
			return 0;
		pt_detect_app();
		if (pt_task && pid_alive(pt_task) && pt_task->mm)
			pt_check_next_addr(pt_task->mm);
		msleep(10);
	}
	
	return 0;
}

