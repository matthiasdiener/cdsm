#include "pt_comm.h"

static int pt_pid[PT_PID_HASH_SIZE];


void pt_delete_pid(int pid, int tid)
{
	unsigned h = hash_32(pid, PT_PID_HASH_BITS);
	if (pt_pid[h] != -1) {
		pt_pid[h] = -1;
		atomic_dec(&pt_active_threads);
		printk("pt: thread %d stop (tid %d)\n", pid, tid);
	}

}


int pt_add_pid(int pid, int tid)
{
	unsigned h = hash_32(pid, PT_PID_HASH_BITS);

	if (pt_pid[h] == -1) {
		pt_pid[h] = tid;
		atomic_inc(&pt_num_threads);
		atomic_inc(&pt_active_threads);
		printk ("pt: added mapping: pid=%d -> tid=%d\n", pid, tid);
		return tid;
	} else {
		printk("pt: XXX thread already registered %d->%d\n", pid, tid);
		return -1;
	}
}


void pt_pid_clear(void)
{
	memset(pt_pid, -1, sizeof(pt_pid));
}


int pt_get_tid(int pid)
{
	unsigned h = hash_32(pid, PT_PID_HASH_BITS);
	return pt_pid[h];
}
