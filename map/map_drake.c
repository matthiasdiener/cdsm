#include "../spcd.h"

struct pos{
	int x, y, max;
};


int findnext(int done[], int nt)
{
	int i;
	for (i=0; i<nt; i++) {
		if (get_share(nt-1, i)>0) {
			return i;
		}
	}

	return -1;
}


struct pos findmax(int v, int done[], int nt)
{
	int max = 0, i = 0, y = 0, tmp;
	struct pos res;

	for (;i<nt; i++) {
		tmp = get_share(v,i);
		if (tmp > max && !done[i]) {
			max = tmp;
			y = i;
		}
	}

	res.max = max;
	res.x = v;
	res.y = y;

	return res;
}


void map_drake(void) {
	int nt = spcd_get_active_threads();
	int v, wL = 0, wR = 0, i = 1, done[nt];
	struct pos ret;

	// char L[256] = "L:", R[256] = "R:";
	printk("test\n");
	memset(done, 0, nt);

	while ((v=findnext(done, nt))!=-1) {
		printk("next vertex: %d\n", v);
		while (1) {
			ret = findmax(v, done, nt);
			if (!ret.max)
				break;
			if (i%2) wL+=ret.max; else wR+=ret.max;
			printk("add to %s: %d (%d,%d)\n", i%2 ? "L" : "R", ret.max, ret.x, ret.y);
			// sprintf(i%2 ? L : R, "%s %d (%d,%d)", i%2 ? L : R, ret.max, ret.x, ret.y);
			done[v] = 1;

			// printk("%s\n", L);
			// printk("%s\n\n", R);
			i++;
			v = ret.y;
		}	
	}
	// printk("Choose: %s (%d)\n", wL>wR ? "L" : "R", wL>wR ? wL : wR);
}
