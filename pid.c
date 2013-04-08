#include "spcd.h"

struct pid_s {
	int pid;
	int tid;
};

static struct pid_s pt_pid[PT_PID_HASH_SIZE];
static struct pid_s pt_pid_reverse[PT_PID_HASH_SIZE];
static atomic_t pt_num_threads = ATOMIC_INIT(0);
static atomic_t pt_active_threads = ATOMIC_INIT(0);


void pt_delete_pid(int pid)
{
	int at;
	unsigned h = hash_32(pid, PT_PID_HASH_BITS);
	int tid = pt_pid[h].tid;

	if (tid != -1 && pt_pid[h].pid == pid) {
		pt_pid[h].tid = -1;
		pt_pid[h].pid = -1;
		//TODO: need to delete from pt_pid_reverse?
		at = atomic_dec_return(&pt_active_threads);
		printk("SPCD: thread %d stop (tid %d), active: %d\n", pid, tid, at);
	}

}


void pt_add_pid(int pid)
{
	unsigned h = hash_32(pid, PT_PID_HASH_BITS);
	int at;

	if (pt_pid[h].pid == -1) {
		pt_pid[h].pid = pid;
		pt_pid[h].tid = atomic_inc_return(&pt_num_threads) - 1;
		at = atomic_inc_return(&pt_active_threads);
		
		pt_pid_reverse[pt_pid[h].tid] = pt_pid[h];
		
		printk ("SPCD: added mapping: pid=%d -> tid=%d, active: %d\n", pid, pt_pid[h].tid, at);
	} else {
		printk("SPCD BUG: XXX pid collision: %d->%d\n", pt_pid[h].pid, pt_pid[h].tid);
	}
}


void pt_pid_clear(void)
{
	memset(pt_pid, -1, sizeof(pt_pid));
	memset(pt_pid_reverse, -1, sizeof(pt_pid_reverse));
	atomic_set(&pt_num_threads, 0);
	atomic_set(&pt_active_threads, 0);
}


int pt_get_pid(int tid){
	return pt_pid_reverse[tid].pid;
}


int pt_get_tid(int pid)
{
	unsigned h = hash_32(pid, PT_PID_HASH_BITS);
	if (pt_pid[h].pid == pid)
		return pt_pid[h].tid;
	return -1;
}


int spcd_get_num_threads(void)
{
	return atomic_read(&pt_num_threads);
}


int spcd_get_active_threads(void)
{
	return atomic_read(&pt_active_threads);
}
