#include "../spcd.h"

struct pos {
	s16 x, y;
	unsigned val;
};

#define SIZE 16

unsigned Mat0[][SIZE] = {
{0,29,4,0,3,1,1,1,3,6,1,5,1,5,2,6 },
{29,0,28,3,1,2,6,3,1,3,1,3,1,3,1,8},
{4,28,0,33,0,0,3,6,0,1,2,3,1,2,2,2},
{0,3,33,0,34,1,1,2,0,3,0,1,2,1,0,1},
{3,1,0,34,0,27,0,0,0,2,1,2,0,3,0,2},
{1,2,0,1,27,0,26,0,1,1,2,2,1,1,0,3},
{1,6,3,1,0,26,0,40,3,3,1,1,0,0,3,3},
{1,3,6,2,0,0,40,0,23,0,1,6,1,3,3,4},
{3,1,0,0,0,1,3,23,0,32,0,0,1,0,1,1},
{6,3,1,3,2,1,3,0,32,0,29,0,0,2,1,1},
{1,1,2,0,1,2,1,1,0,29,0,28,6,0,2,2},
{5,3,3,1,2,2,1,6,0,0,28,0,22,2,0,1},
{1,1,1,2,0,1,0,1,1,0,6,22,0,25,0,5},
{5,3,2,1,3,1,0,3,0,2,0,2,25,0,30,1},
{2,1,2,0,0,0,3,3,1,1,2,0,0,30,0,31},
{6,8,2,1,2,3,3,4,1,1,2,1,5,1,31,0 },
};

unsigned Mat1[SIZE][SIZE] = {};

static inline unsigned get_share_xxx(int x, int y, int level)
{
	switch (level) {
		case 0:
			return Mat0[x][y];
		case 1:
			return Mat1[x][y];
		default:
			return 0;
	}
}

static inline void printMat(int level)
{
	int i,j, nt=SIZE;

	for (i = nt-1; i >= 0; i--) {
		for (j = 0; j < nt; j++) {
			int s = get_share_xxx(i, j, level);
			printk ("%u", s);
			if (j != nt-1)
				printk (",");
		}
		printk("\n");
	}

}


int findnext(int done[], int nt, int level)
{
	int i;
	// printk("done[] findnext: %d %d %d %d\n", done[0], done[1], done[2], done[3]);
	for (i=0; i<nt; i++) {
		if (!done[i] && get_share_xxx(nt-1, i, level)>0) {
			// printk("\tfindnext(%d,%d) == %d\n", nt-1, i, get_share_xxx(nt-1, i));
			return i;
		}
	}

	return -1;
}


struct pos findmax(int v, int done[], int nt, int level)
{
	int i, tmp;
	struct pos res = {.x=v, .val=0};
	// printk("done[] findmax: %d %d %d %d\n", done[0], done[1], done[2], done[3]);

	for (i=0; i<nt; i++) {
		tmp = get_share_xxx(v, i, level);
		// printk("\tfindmax(%d,%d) == %d\n", v, i, tmp);
		if (tmp > res.val && !done[i]) {
			res.val = tmp;
			res.y = i;
		}
	}

	return res;
}

void generate_new_matrix(int newlevel)
{

}

static inline void add_res(struct pos r[][2][SIZE], int level, int i, struct pos ret)
{
	int j=0;
	while (r[level][i][j].val!=0)
		j++;
	r[level][i][j] = ret;
	printk("added: i:%d idx:%d w:%d (%d,%d)\n", i, j, r[level][i][j].val, r[level][i][j].x, r[level][i][j].y);
}


void map_drake(int ntxxx) {
	int nt=SIZE; //debug
	int v, i=0, done[nt];
	struct pos ret;
	char M[2][256];
	int W[2] = {0};
	struct pos res[4][2][nt];

	sprintf(M[0], " ");
	sprintf(M[1], " ");

	printMat(0);

	memset(done, 0, nt*sizeof(int));
	memset(res, 0, nt*2*4*sizeof(struct pos));
	printk("drake start\n");

	while ((v=findnext(done, nt, 0))!=-1) {
		// printk("\tnext vertex: %d\n", v);
		while (1) {
			ret = findmax(v, done, nt, 0);
			done[v] = 1;

			// printk("\tfindmax: max=%d, x=%d, y=%d\n", ret.max, ret.x, ret.y);
			if (!ret.val)
				break;
			W[i] += ret.val;
			add_res(res, 0, i, ret);

			// printk("\tadd to M[%d]: %d (%d,%d)\n", i, ret.val, ret.x, ret.y);
			sprintf(M[i], "%s %d (%d,%d) ", M[i], ret.val, ret.x, ret.y);
			
			// printk("\tM[0]: %s\n", M[0]);
			// printk("\tM[1]: %s\n", M[1]);
			i = 1-i;
			v = ret.y;
		}
	}
	i=0;
	printk("Result:\nM[0]:");
	while (res[0][0][i].val!=0) {
		printk(" %d (%d,%d) ", res[0][0][i].val, res[0][0][i].x, res[0][0][i].y);
		i++;
	}
	i=0;
	printk("\nM[1]:");
	while (res[0][1][i].val!=0) {
		printk(" %d (%d,%d) ", res[0][1][i].val, res[0][1][i].x, res[0][1][i].y);
		i++;
	}

	printk("\nChoose: M[%d] (%d)\n", W[0]>W[1] ? 0 : 1, W[0]>W[1] ? W[0] : W[1]);

	generate_new_matrix(1);
	printMat(1);
}
