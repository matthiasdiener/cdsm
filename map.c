#include "spcd.h"


static unsigned sharecp [PT_MAXTHREADS][PT_MAXTHREADS];
static int pos_x, pos_y;

static void clear(int x, int y, int nt)
{
	int i;

	for (i = 0; i<nt; i++) {
		sharecp[x][i] = 0;
		sharecp[y][i] = 0;
		sharecp[i][x] = 0;
		sharecp[i][y] = 0;
	}
}

static unsigned listgetmax(int nt)
{
	unsigned res = 0;
	int i, j;

	for (i = nt-1; i >= 0; i--) {
		for (j = 0; j < nt; j++) {
			if (sharecp[i][j] + sharecp[j][i] > res) {
				res = sharecp[i][j]+sharecp[j][i];
				pos_x = i;
				pos_y = j;
			}
		}
	}
	clear(pos_x, pos_y, nt);
	return res;
}

void check_map(int nt)
{
	unsigned max;
	memcpy(sharecp, share, sizeof(share));

	while (1) {
		max = listgetmax(nt);
		if (max == 0)
			break;
		printk("(%d,%d) ", pos_x, pos_y);
	}
	printk("\n");
}

int spcd_map_func(void* v)
{
	int nt;
	while (1) {
		if (kthread_should_stop())
			return 0;
		
		nt = spcd_get_active_threads();
		if (nt > 1) {
			// pt_print_share();
			check_map(nt);
			//pt_share_clear();
		}
		msleep(100);
	}
}