#include "pt_comm.h"

void pt_get_children(void)
{
	struct task_struct *task, *child;
	struct list_head *pos;

	// // child_task = list_entry(&children_list,struct task_struct,children);
	// // list_for_each(tmp, &children_list) {
	// child_task = list_entry(&children_list, struct task_struct, sibling);
	// printk("I am parent: %u, my child is: %u \n", tsk->pid,child_task->pid);
	// // }

	for_each_process(task)
    {
    	// printk("%s [%d]\n",task->comm , task->pid);
    	// child_task = list_entry(&children_list,struct task_struct, children);
    	// printk("KERN_INFO I am parent: %d, my child is: %d \n",task->pid, child_task->pid);
    	list_for_each(pos, &task->sibling)
    	{
    		child = list_entry(pos, struct task_struct, sibling);
    		printk("%s: %d, %s: %d , %p, %p\n",task->comm, task->pid, child->comm, child->pid, task->mm, child->mm);
    	}
    }
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

