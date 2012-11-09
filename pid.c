#include "spcd.h"

static int pt_pid[PT_PID_HASH_SIZE];
static atomic_t pt_num_threads = ATOMIC_INIT(0);
static atomic_t pt_active_threads = ATOMIC_INIT(0);


void pt_delete_pid(int pid)
{
	unsigned h = hash_32(pid, PT_PID_HASH_BITS);
	int tid = pt_pid[h];
	int at;

	if (tid != -1) {
		pt_pid[h] = -1;
		at = atomic_dec_return(&pt_active_threads);
		printk("pt: thread %d stop (tid %d), active: %d\n", pid, tid, at);
	}

}


void pt_add_pid(int pid)
{
	unsigned h = hash_32(pid, PT_PID_HASH_BITS);
	int at;

	if (pt_pid[h] == -1) {
		pt_pid[h] = atomic_inc_return(&pt_num_threads) - 1;
		at = atomic_inc_return(&pt_active_threads);
		printk ("pt: added mapping: pid=%d -> tid=%d, active: %d\n", pid, pt_pid[h], at);
	} else {
		printk("pt: XXX thread already registered %d->%d\n", pid, pt_pid[h]);
	}
}


void pt_pid_clear(void)
{
	memset(pt_pid, -1, sizeof(pt_pid));
	atomic_set(&pt_num_threads, 0);
	atomic_set(&pt_active_threads, 0);
}


int pt_get_tid(int pid)
{
	return pt_pid[hash_32(pid, PT_PID_HASH_BITS)];
}


int spcd_get_num_threads(void) {
	return atomic_read(&pt_num_threads);
}


int spcd_get_active_threads(void) {
	return atomic_read(&pt_active_threads);
}
