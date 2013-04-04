#include "libspcd.h"

void *spcd_matrix_encode(spcd_matrix_t *data)
{
	spcd_matrix_t *tmp_matrix;
	void *tmp, *ret;
#ifdef __KERNEL__
	ret = kmalloc(sizeof(unsigned) + 
				 sizeof(spcd_matrix_t) + 
				 (sizeof(int) * data->num_threads) + 
				 (sizeof(unsigned) * data->max_threads * data->max_threads), GFP_KERNEL);
#else
	ret = malloc(sizeof(unsigned) + 
				 sizeof(spcd_matrix_t) + 
				 (sizeof(int) * data->num_threads) + 
				 (sizeof(unsigned) * data->max_threads * data->max_threads));
#endif	
	
	*((unsigned*)ret) = SPCD_MAGIC;
	tmp = ret + sizeof(unsigned);
	tmp_matrix = tmp;
	memcpy(tmp, data, sizeof(spcd_matrix_t));
	tmp_matrix->pids = NULL;
	tmp_matrix->matrix = NULL;
	
	tmp += sizeof(spcd_matrix_t);
	memcpy(tmp, data->pids, sizeof(int) * data->num_threads);
	tmp += sizeof(int) * data->num_threads;
	memcpy(tmp, data->matrix, (sizeof(unsigned) * data->max_threads * data->max_threads));

	return ret;
}


spcd_matrix_t spcd_matrix_decode(void *ptr)
{
	spcd_matrix_t ret;
	void *tmp;
	
	if(*((unsigned*) ptr) != SPCD_MAGIC){
		ret.num_threads = -1;
		return ret;
	}
	
	tmp = ptr + sizeof(unsigned);	
	
	memcpy(&ret, tmp, sizeof(spcd_matrix_t));
#ifdef __KERNEL__
	ret.pids = kmalloc(sizeof(int) * ret.num_threads, GFP_KERNEL);
	ret.matrix = kmalloc(sizeof(unsigned) * ret.max_threads * ret.max_threads, GFP_KERNEL);
#else
	ret.pids = malloc(sizeof(int) * ret.num_threads);
	ret.matrix = malloc(sizeof(unsigned) * ret.max_threads * ret.max_threads);	
#endif		
	tmp += sizeof(spcd_matrix_t);
	memcpy(ret.pids, tmp, sizeof(int) * ret.num_threads);
	
	tmp += sizeof(int) * ret.num_threads;
	memcpy(ret.matrix, tmp, sizeof(unsigned) * ret.max_threads * ret.max_threads);

	return ret;
}


size_t spcd_matrix_size(void *ptr)
{
	spcd_matrix_t tmp = spcd_matrix_decode(ptr);
	return sizeof(unsigned) + 
		   sizeof(spcd_matrix_t) + 
		   (sizeof(int) * tmp.num_threads) + 
		   (sizeof(unsigned) * tmp.max_threads * tmp.max_threads);
}

#ifndef __KERNEL__
unsigned *spcd_get_small_matrix(spcd_matrix_t *data)
{
	int i,j,a=0;
	unsigned *ret = malloc(sizeof(unsigned) * data->num_threads * data->num_threads);
	for (i = data->num_threads-1; i >= 0; i--) {
		for (j = 0; j < data->num_threads; j++) {
			ret[a] = i > j ? data->matrix[i*data->max_threads + j] : data->matrix[j*data->max_threads + i];
			a++;
		}
	}
	
	return ret;
}


void spcd_matrix_print(spcd_matrix_t *data)
{
	int i, j;
	
	if(data == NULL){
		printf("NULL\n");
	}
	else{
		printf("Threads: %d\n", data->num_threads);
		printf("Max Threads: %d\n", data->max_threads);
		printf("PIDs: ");
		for(i=0;i<data->num_threads;i++){
			printf("%d", data->pids[i]);
			if(i != data->num_threads - 1)
				printf(",");
		}
		printf("\nMatrix:");
		for(j=0;j<data->num_threads;j++){
			printf("\n\t");
			for(i=0;i<data->num_threads;i++){
				printf("%d", spcd_get_small_matrix(data)[(j*data->num_threads) + i]);
				if(i != data->num_threads - 1)
					printf(",");
			}
		}
		printf("\n");
	}
}
#endif
