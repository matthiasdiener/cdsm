#include "pt_comm.h"

static int pt_pid[PT_PID_HASH_SIZE];


static int pt_add_pid(int pid, int tid)
{
	unsigned h = hash_32(pid, PT_PID_HASH_BITS);

	if (pt_pid[h] == -1) {
		pt_pid[h] = tid;
		printk ("pt: added mapping: pid=%d -> tid=%d\n", pid, tid);
		return tid;
	} else {
		printk("pt: XXX thread already registered %d->%d\n", pid, tid);
		return -1;
	}
}


static void pt_pid_clear(void)
{
	memset(pt_pid, -1, sizeof(pt_pid));
}


static int pt_get_tid(int pid)
{
	unsigned h = hash_32(pid, PT_PID_HASH_BITS);
	return pt_pid[h];
}
