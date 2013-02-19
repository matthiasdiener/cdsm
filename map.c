#include "spcd.h"

static int pos_x, pos_y;
static u8 *mapped;


static inline void mark(int x, int y)
{
	mapped[x] = 1;
	mapped[y] = 1;
}

static unsigned listgetmax(int nt)
{
	int i, j;
	unsigned res = 0;
	unsigned s;

	for (i = nt-1; i >= 0; i--) {
		if (mapped[i])
			continue;
		for (j = 0; j < nt; j++) {
			if (mapped[j])
				continue;
			s = get_share(i,j);
			if (s > res) {
				res = s;
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
	memset(mapped, 0, sizeof(u8)*max_threads);

	while (listgetmax(nt)) {
		// printk("(%d,%d) ", pos_x, pos_y);
	}

	// printk("\n");
}


int spcd_map_func(void* v)
{
	int nt;
	mapped = kmalloc(max_threads*sizeof(u8), GFP_KERNEL);
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
	kfree(mapped);
}
