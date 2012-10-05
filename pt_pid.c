#include "pt_comm.h"

static int pt_pid[1024];

void pt_add_pid(int pid, int tid)
{
	unsigned h = hash_32(pid, 10);
	pt_pid[h] = tid;
	printk ("### added mapping: pid=%d -> tid=%d\n", pid, tid);
}


void pt_pid_clear(void)
{
	int i;
	for (i=0; i<sizeof(pt_pid); i++)
		pt_pid[i] = -1;

}


int pt_get_numthreads(void)
{
	int i, res = 0;
	for (i=0; i<sizeof(pt_pid); i++)
		if (pt_pid[i] !=-1)
			res++;

	return res;
}


int pt_get_tid(int pid)
{
	unsigned h = hash_32(pid, 10);
	return pt_pid[h];
}