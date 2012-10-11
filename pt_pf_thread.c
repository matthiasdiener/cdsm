#include "pt_comm.h"

static int pt_pf_thread_func(void* v)
{
	
	while (1) {
		if (kthread_should_stop())
			return 0;
		if (pt_task && pid_alive(pt_task) && pt_task->mm)
			pt_pf_pagewalk(pt_task->mm);
		msleep(10);
	}
	
}
