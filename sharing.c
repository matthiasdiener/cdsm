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


static int avg(unsigned share[PT_MAXTHREADS][PT_MAXTHREADS], int nt)
{
	int i, j;
	int sum = 0;
	for (i = nt-1; i >= 0; i--) {
		for (j = 0; j < nt; j++) {
			sum += share[i][j] + share[j][i];
		}
	}
	return sum / nt / nt;
}


static int var(unsigned share[PT_MAXTHREADS][PT_MAXTHREADS], int nt, int avg)
{
	int i, j;
	int sum = 0;

	for (i = nt-1; i >= 0; i--) {
		for (j = 0; j < nt; j++) {
			int diff = avg - (share[i][j] + share[j][i]);
			sum += diff*diff;
		}
	}
	return sum / nt / nt;
}


void pt_print_share(void)
{
	int i, j;
	int nt = spcd_get_num_threads();
	int av, va;

	for (i = nt-1; i >= 0; i--) {
		for (j = 0; j < nt; j++) {
			printk ("%u", share[i][j] + share[j][i]);
			if (j != nt-1)
				printk (",");
		}
		printk("\n");
	}
	
	av = avg(share, nt);
	va = var(share, nt, av);

	printk ("avg: %d, var: %d, hf: %d\n", av, va, va/av);
}


void pt_share_clear(void)
{
	memset(share, 0, sizeof(share));
}
