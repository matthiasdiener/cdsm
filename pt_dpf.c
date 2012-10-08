#include "pt_comm.h"



void pt_check_comm(struct task_struct *tsk, unsigned long address)
{
	DEFINE_SPINLOCK(ptl);
	int mytid;
	struct pt_mem_info *elem;

	// detection not requested -> return
	if (pt_task == 0)
		return;
	
	mytid = pt_get_tid(tsk->pid);
	// thread not in list -> we are not interested
	if (mytid == -1)
		return;


	elem = pt_get_mem(address);

	spin_lock(&ptl);

	if (elem->pg_addr != (address >> PAGE_SHIFT) ){
		if (elem->pg_addr !=0 ) {
			// printk ("XXX conflict, hash = %ld, old = %lu, new = %lu\n", h, elem->pg_addr, (address >> PAGE_SHIFT));
			pt_addr_conflict++;
		}
		elem->sharer[0] = -1;
		elem->sharer[1] = -1;
		elem->pg_addr = address >> PAGE_SHIFT;
	}

	// no sharer present
	if (elem->sharer[0] == -1 && elem->sharer[1] == -1) {
		elem->sharer[0] = mytid;
		goto out;
	}

	// two sharers present
	if (elem->sharer[0] != -1 && elem->sharer[1] != -1) {

		// both different from myself
		if (elem->sharer[0] != mytid && elem->sharer[1] != mytid) {
			share[mytid][elem->sharer[0]] ++;
			share[mytid][elem->sharer[1]] ++;
			
			elem->sharer[1] = elem->sharer[0];
			elem->sharer[0] = mytid;

			goto out;
		}


		if (elem->sharer[0] == mytid) {
			share[mytid][elem->sharer[1]] ++;
			goto out;
		}

		if (elem->sharer[1] == mytid) {
			share[mytid][elem->sharer[0]] ++;
			goto out;
		}


		goto out;
	}

	// only second sharer present
	if (elem->sharer[0] == -1) {
		if (elem->sharer[1] != mytid) {
			elem->sharer[0] = mytid;
			share[mytid][elem->sharer[1]] ++;
		}
		goto out;
	}

	// only first sharer present
	if (elem->sharer[0] != mytid) {
		elem->sharer[1] = mytid;
		share[mytid][elem->sharer[0]] ++;
	}

	out:
		spin_unlock(&ptl);

}


void pt_print_comm(void)
{
	int i,j;
	int nt = pt_get_numthreads();
	for (i = nt-1; i >= 0; i--) {
		for (j = 0; j < nt; j++){
			printk ("%lu", share[i][j]+share[j][i]);
			if (j != nt-1)
				printk (",");
		}
		printk("\n");
	}
}