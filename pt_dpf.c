#include "pt_comm.h"

struct pt_mem_info* pt_check_comm(int tid, unsigned long address)
{
	struct pt_mem_info *elem = pt_get_mem_init(address);

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
	return elem;
}
