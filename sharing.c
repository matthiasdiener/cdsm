#include "spcd.h"

unsigned share [PT_MAXTHREADS][PT_MAXTHREADS];

static inline int get_num_sharers(struct pt_mem_info *elem)
{
	if (elem->sharer[0] == -1 && elem->sharer[1] == -1)
		return 0;
	if (elem->sharer[0] != -1 && elem->sharer[1] != -1)
		return 2;
	return 1;
}


void pt_check_comm(int tid, unsigned long address)
{
	struct pt_mem_info *elem = pt_get_mem_init(address);

	int sh = get_num_sharers(elem);

	switch (sh) {
		case 0:
			elem->sharer[0] = tid;
			break;

		case 1:
			if (elem->sharer[0] == -1) {
				if (elem->sharer[1] != tid) {
					elem->sharer[0] = tid;
					share[tid][elem->sharer[1]] ++;
				}
			} else if (elem->sharer[0] != tid) {
				elem->sharer[1] = tid;
				share[tid][elem->sharer[0]] ++;
			}
			break;

		case 2:
			if (elem->sharer[0] != tid && elem->sharer[1] != tid) {
				share[tid][elem->sharer[0]] ++;
				share[tid][elem->sharer[1]] ++;

				elem->sharer[1] = elem->sharer[0];
				elem->sharer[0] = tid;
			} else if (elem->sharer[0] == tid) {
				share[tid][elem->sharer[1]] ++;
			} else if (elem->sharer[1] == tid) {
				share[tid][elem->sharer[0]] ++;
			}

			break;
	}

}


void pt_print_share(void)
{
	int i, j;
	int nt = spcd_get_num_threads();

	for (i = nt-1; i >= 0; i--) {
		for (j = 0; j < nt; j++) {
			printk ("%u", share[i][j] + share[j][i]);
			if (j != nt-1)
				printk (",");
		}
		printk("\n");
	}
}


void pt_share_clear(void)
{
	memset(share, 0, sizeof(share));
}
