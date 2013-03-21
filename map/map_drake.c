#include "../spcd.h"

struct pos{
	int x, y, max;
};


int findnext(int done[], int nt)
{
	int i;
	// printk("done[] findnext: %d %d %d %d\n", done[0], done[1], done[2], done[3]);
	for (i=0; i<nt; i++) {
		if (!done[i] && get_share(nt-1, i)>0) {
			// printk("\tfindnext(%d,%d) == %d\n", nt-1, i, get_share(nt-1, i));
			return i;
		}
	}

	return -1;
}


struct pos findmax(int v, int done[], int nt)
{
	int i, tmp;
	struct pos res = {.x=v, .max=0};
	// printk("done[] findmax: %d %d %d %d\n", done[0], done[1], done[2], done[3]);

	for (i=0; i<nt; i++) {
		tmp = get_share(v, i);
		// printk("\tfindmax(%d,%d) == %d\n", v, i, tmp);
		if (tmp > res.max && !done[i]) {
			res.max = tmp;
			res.y = i;
		}
	}

	return res;
}


void map_drake(int nt) {
	int v, i = 0, done[nt];
	struct pos ret;
	// char M[2][256];
	int W[2] = {0};

	// sprintf(M[0], " ");
	// sprintf(M[1], " ");

	memset(done, 0, nt*sizeof(int));
	// printk("drake start\n");

	while ((v=findnext(done, nt))!=-1) {
		// printk("\tnext vertex: %d\n", v);
		while (1) {
			ret = findmax(v, done, nt);
			done[v] = 1;

			// printk("\tfindmax: max=%d, x=%d, y=%d\n", ret.max, ret.x, ret.y);
			if (!ret.max)
				break;
			W[i] += ret.max;
			// printk("\tadd to M[%d]: %d (%d,%d)\n", i, ret.max, ret.x, ret.y);
			// sprintf(M[i], "%s %d (%d,%d) ", M[i], ret.max, ret.x, ret.y);
			
			// printk("\tM[0]: %s\n", M[0]);
			// printk("\tM[1]: %s\n", M[1]);
			i = 3-i-2;
			v = ret.y;
		}
	}

	printk("Choose: M[%d] (%d)\n", W[0]>W[1] ? 0 : 1, W[0]>W[1] ? W[0] : W[1]);
}
