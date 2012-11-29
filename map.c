#include "spcd.h"

static int pos_x, pos_y;
static u8 mapped [PT_MAXTHREADS];


static inline void mark(int x, int y)
{
	mapped[x] = 1;
	mapped[y] = 1;
}

static unsigned listgetmax(int nt)
{
	int i, j;
	unsigned res = 0;

	for (i = nt-1; i >= 0; i--) {
		if (mapped[i])
			continue;
		for (j = 0; j < nt; j++) {
			if (mapped[j])
				continue;
			if (share[i][j] + share[j][i] > res) {
				res = share[i][j]+share[j][i];
				pos_x = i;
				pos_y = j;
			}
		}
	}
	mark(pos_x, pos_y);
	return res;
}


void check_map(int nt)
{
	memset(mapped, 0, sizeof(mapped));

	while (listgetmax(nt)) {
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
