#include <unistd.h>
#include <stdio.h>

#define PT_MAXTHREADS 4096

unsigned share [PT_MAXTHREADS][PT_MAXTHREADS];

int main (void)
{
	int i, j, nt;

	nt = syscall(177, share, 1);
	printf("Threads: %d\n", nt);

	for (i = nt-1; i >= 0; i--) {
		for (j = 0; j < nt; j++) {
			printf ("%u", share[i][j] + share[j][i]);
			if (j != nt-1)
				printf (",");
		}
		printf("\n");
	}
}
