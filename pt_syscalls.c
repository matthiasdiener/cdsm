static struct task_struct *pt_thr;

SYSCALL_DEFINE0(pt_detect_start)
{
	struct task_struct *tsk = current;
	pt_mm_to_detect = tsk->mm;
	pt_extra_pf = 0;
	addr_conflict = 0;
	num_faults = 3;

	// in case last execution crashed:
	// pt_mem_delete();
	pt_pid_delete();
	memset(share, 0, sizeof (share));

	if (pt_thr == 0) {
		pt_thr = kthread_create(pt_timer_func, NULL, "pt_timer");
	}

	wake_up_process(pt_thr);

	printk("### pt: detect_comm start, pid=%d\n", tsk->pid);
 
 	pt_num_walks = 0;
 	pt_next_vma = NULL;
	n = 0;

	return 0;
}


SYSCALL_DEFINE1(pt_numthreads, int, mytid)
{
	pt_add_pid (current->pid, mytid);
	return 0;
}


SYSCALL_DEFINE0(pt_detect_stop)
{

	pt_mm_to_detect = 0;

	pt_mem_delete();

	printk("### pt: detect_comm stop, page faults=%lu (%lu extra), #threads=%d, addr conflicts=%lu, num_walks=%lu\n", n, pt_extra_pf, pt_get_numthreads(), addr_conflict, num_walks-1);

	pt_print_comm();
	pt_num_walks = 0;


	return 0;
}


SYSCALL_DEFINE1(pt_get_comm, unsigned long *, addr)
{
	int ret = copy_to_user(addr, share, sizeof (share)) ? -EFAULT : 0;
	memset(share, 0, sizeof (share));
	return ret;
}


SYSCALL_DEFINE0(pt_reset_comm)
{
	memset(share, 0, sizeof (share));
	return 0;
}