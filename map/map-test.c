#include <stdio.h>
#include <stdint.h>
#include <string.h>

struct pos{
	int x, y, max;
};

#define SIZE 7

int M[][SIZE] = {
// {0,29,4,0,3,1,1,1,3,6,1,5,1,5,2,6},
// {29,0,28,3,1,2,6,3,1,3,1,3,1,3,1,8},
// {4,28,0,33,0,0,3,6,0,1,2,3,1,2,2,2},
// {0,3,33,0,34,1,1,2,0,3,0,1,2,1,0,1},
// {3,1,0,34,0,27,0,0,0,2,1,2,0,3,0,2},
// {1,2,0,1,27,0,26,0,1,1,2,2,1,1,0,3},
// {1,6,3,1,0,26,0,40,3,3,1,1,0,0,3,3},
// {1,3,6,2,0,0,40,0,23,0,1,6,1,3,3,4},
// {3,1,0,0,0,1,3,23,0,32,0,0,1,0,1,1},
// {6,3,1,3,2,1,3,0,32,0,29,0,0,2,1,1},
// {1,1,2,0,1,2,1,1,0,29,0,28,6,0,2,2},
// {5,3,3,1,2,2,1,6,0,0,28,0,22,2,0,1},
// {1,1,1,2,0,1,0,1,1,0,6,22,0,25,0,5},
// {5,3,2,1,3,1,0,3,0,2,0,2,25,0,30,1},
// {2,1,2,0,0,0,3,3,1,1,2,0,0,30,0,31},
// {6,8,2,1,2,3,3,4,1,1,2,1,5,1,31,0},
{0,32,22,2,2,2,0    },
{32,0,47,541,2,472,0},
{22,47,0,0,567,0,547},
{2,541,0,0,0,570,0  },
{2,2,567,0,0,0,514  },
{2,472,0,570,0,0,0  },
{0,0,547,0,514,0,0  },
};


void print_matrix(void)
{
	int i,j;
	for (i=0; i<SIZE; i++) {
		for (j=0; j<SIZE; j++) {
			printf ("%d", M[i][j]);
			if (j != SIZE-1)
				printf (",");
		}
		printf("\n");
	}
}

int findnext(int done[])
{
	int i;
	for (i=0; i<SIZE; i++) {
		if (M[SIZE-1][i]>0 && !done[i]) {
			return i;
		}
	}

	return -1;
}

struct pos findmax(int v, int done[])
{
	int max = 0, i = 0, y = 0;
	struct pos res;

	for (;i<SIZE; i++) {
		if (M[v][i] > max && !done[i]) {
			max = M[v][i];
			y = i;
		}
	}

	res.max = max;
	res.x = v;
	res.y = y;

	return res;
}


void map(void) {
	int v, wL = 0, wR = 0, i = 1, done[SIZE] = {0};
	struct pos ret;

	char L[512] = "L:", R[512] = "R:";

	while ((v=findnext(done))!=-1) {
		printf("next vertex: %d\n", v);
		while (1) {
			ret = findmax(v, done);
			if (!ret.max)
				break;
			if (i%2) wL+=ret.max; else wR+=ret.max;
			printf("add to %s: %d (%d,%d)\n", i%2 ? "L" : "R", ret.max, ret.x, ret.y);
			sprintf(i%2 ? L : R, "%s %d (%d,%d)", i%2 ? L : R, ret.max, ret.x, ret.y);
			done[v] = 1;

			printf("%s\n", L);
			printf("%s\n\n", R);
			i++;
			v = ret.y;
		}	
	}
	printf("Choose: %s (%d)\n", wL>wR ? "L" : "R", wL>wR ? wL : wR);
}


int main (int argc, char *argv[])
{	
	map();
	map();
	return 0;
}
