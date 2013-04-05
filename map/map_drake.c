#include "../spcd.h"

#define SIZE 16 //debug
#define LEVELS 4


struct pos {
	unsigned val;
	s16 id[SIZE]; //needs to be array
};



unsigned Mat[4][SIZE][SIZE] = {
{
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
},
 {}
};

unsigned sizes[LEVELS] = {SIZE};
unsigned arity[LEVELS] = {4,2,2};


static inline
unsigned get_share_xxx(int x, int y, int level)
{
	return Mat[level][x][y];
}

static inline
void printMat(int level)
{
	int i,j, nt=sizes[level];

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


static inline
int findnext(int done[], int nt, int level)
{
	int i;

	for (i=0; i<nt; i++) {
		if (!done[i] && get_share_xxx(i==0 ? 1 : i-1, i, level)>0) {
			printk("findnext: %d\n", i);
			return i;
		}
	}

	return -1;
}


static inline
struct pos findmax(int v, int done[], int nt, int level)
{
	int i, j, tmp, p = 1, max = 0, maxi = 0;
	struct pos res = {.id={v, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, .val=0};
	printk("  findmax(%d) start\n", v);
	for (j=0; j<arity[level]-1; j++) {

		for (i=0; i<nt; i++) {
			tmp = get_share_xxx(v, i, level);
			printk("    %d: %d\n", i, tmp);
			if (tmp > max && !done[i]) {
				max = tmp;
				maxi = i;
			}
		}
		if (max) {
			printk("    choose: %d\n", maxi);
			res.val += max;
			res.id[p++] = maxi;
			done[maxi] = 1;
			max = 0;
		}
	}

	printk("  findmax(%d): ", v);
	for (j=0; j<SIZE; j++)
		printk("%d ", res.id[j]);
	
	printk("(val: %d)\n", res.val);
	return res;
}


static inline
void generate_new_matrix(int new, struct pos res[][2][SIZE], int w)
{
	// int old = new-1;
	// int i=0, j, x, y;

	// while (res[old][w][i].val!=0) {
	// 	x = res[old][w][i].x;
	// 	y = res[old][w][i].y;
	// 	printk("xy: (%d,%d)\n", x, y);
	// 	for (j=0; j<i; j++) {
	// 		int x2, y2;
	// 		x2 = res[old][w][j].x;
	// 		y2 = res[old][w][j].y;
	// 		Mat[new][i][j] = get_share_xxx(x, x2, old);
	// 		Mat[new][i][j] += get_share_xxx(x, y2, old);
	// 		Mat[new][i][j] += get_share_xxx(y, x2, old);
	// 		Mat[new][i][j] += get_share_xxx(y, y2, old);
	// 		printk("ij: %d,%d calc new: %d,%d,%d,%d = %d\n", i, j, x, y, x2, y2, Mat[new][i][j]);
	// 		Mat[new][j][i] = Mat[new][i][j]; //make matrix symmetric
	// 	}
	// 	i++;
	// }

	// sizes[new] = i;

}


static inline
void add_res(struct pos r[][2][SIZE], int level, int i, struct pos ret)
{
	int j=0;
	while (r[level][i][j].val!=0)
		j++;
	r[level][i][j] = ret;
	// printk("added: i:%d idx:%d w:%d (%d,%d)\n", i, j, r[level][i][j].val, r[level][i][j].x, r[level][i][j].y);
}


static
void do_drake(struct pos res[][2][SIZE], int done[], int nt, int W[], int level) {
	int v, i=0;
	struct pos ret;

	while ((v=findnext(done, nt, level))!=-1) {
		while (1) {
			ret = findmax(v, done, nt, level);
			done[v] = 1;

			if (!ret.val)
				break;
			W[i] += ret.val;
			add_res(res, level, i, ret);

			i = 1-i;
			v = ret.id[1];
		}
	}
}


void map_drake(int ntxxx) {
	int nt = SIZE; //debug
	int i, j, level;
	int W[LEVELS][2] = {{0}};
	int done[LEVELS][nt];
	struct pos res[LEVELS][2][nt];

	memset(done, 0, LEVELS*nt*sizeof(int));
	memset(res, 0, LEVELS*2*nt*sizeof(struct pos));

	for (level=0; level<1; level++) {
		printk("drake level %d start (arity %d)\n", level, arity[level]);
		// if (level) generate_new_matrix(level, res, W[level-1][0]>W[level-1][1] ? 0 : 1);
		printMat(level);

		do_drake(res, done[level], nt, W[level], level);

		for (j=0; j<2; j++) {
			printk("M[%d]:", j);
			for (i=0; res[level][j][i].val!=0; i++) {
				printk(" %d (%d,%d)", res[level][j][i].val, res[level][j][i].id[0], res[level][j][i].id[1]);
			}
			if (j!=1) printk("\n");
		}

		printk("\nChoose: M[%d] (%d)\n", W[level][0]>W[level][1] ? 0 : 1, W[level][0]>W[level][1] ? W[level][0] : W[level][1]);


	}

}
