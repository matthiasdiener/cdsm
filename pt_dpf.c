#include "pt_comm.h"

struct pt_mem_info* pt_check_comm(int tid, unsigned long address)
{
	DEFINE_SPINLOCK(ptl);
	struct pt_mem_info *elem = pt_get_mem(address);
	
	spin_lock(&ptl);

	if (elem->pg_addr != (address >> PAGE_SHIFT) ){
		if (elem->pg_addr !=0 ) {
			if (elem->pte_cleared){
				pt_fix_pte(address);
			}
			printk ("XXX conflict, hash = %u, old = %lu, new = %lu\n", hash_32(address >> PAGE_SHIFT, PT_MEM_HASH_BITS), elem->pg_addr, (address >> PAGE_SHIFT));
			pt_addr_conflict++;
		}
		elem->sharer[0] = tid;
		elem->sharer[1] = -1;
		elem->pg_addr = address >> PAGE_SHIFT;
		goto out;
	}

	// no sharer present
	if (elem->sharer[0] == -1 && elem->sharer[1] == -1) {
		elem->sharer[0] = tid;
		goto out;
	}

	// two sharers present
	if (elem->sharer[0] != -1 && elem->sharer[1] != -1) {

		// both different from myself
		if (elem->sharer[0] != tid && elem->sharer[1] != tid) {
			share[tid][elem->sharer[0]] ++;
			share[tid][elem->sharer[1]] ++;
			
			elem->sharer[1] = elem->sharer[0];
			elem->sharer[0] = tid;

			goto out;
		}


		if (elem->sharer[0] == tid) {
			share[tid][elem->sharer[1]] ++;
			goto out;
		}

		if (elem->sharer[1] == tid) {
			share[tid][elem->sharer[0]] ++;
			goto out;
		}


		goto out;
	}

	// only second sharer present
	if (elem->sharer[0] == -1) {
		if (elem->sharer[1] != tid) {
			elem->sharer[0] = tid;
			share[tid][elem->sharer[1]] ++;
		}
		goto out;
	}

	// only first sharer present
	if (elem->sharer[0] != tid) {
		elem->sharer[1] = tid;
		share[tid][elem->sharer[0]] ++;
	}

	out:
	spin_unlock(&ptl);
	return elem;
}
