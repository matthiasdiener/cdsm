#ifndef LIBSPCD_H
#define LIBSPCD_H

#define SPCD_MAGIC 0x48151623

#ifdef __KERNEL__
	#include <linux/slab.h>
#else
	#include <string.h>
	#include <stdlib.h>
	#include <stdio.h>
#endif

typedef struct spcd_matrix{
	int num_threads;
	int max_threads;
	int *pids;
	unsigned *matrix;
} spcd_matrix_t;

void *spcd_matrix_encode(spcd_matrix_t *data);
spcd_matrix_t spcd_matrix_decode(void *ptr);
size_t spcd_matrix_size(void *ptr);

#ifndef __KERNEL__
unsigned *spcd_get_small_matrix(spcd_matrix_t *data);
void spcd_matrix_print(spcd_matrix_t *data);
#endif

#endif /* end of include guard: LIBSPCD_H */
