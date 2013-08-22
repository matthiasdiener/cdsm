#include "libmapping.h"

uint64_t libmapping_get_next_power_of_two (uint64_t v)
{
	uint64_t r;
	for (r=1; r<v; r<<=1);
	return r;
}

uint8_t libmapping_get_log2 (uint64_t v)
{
	uint8_t r;
	for (r=0; (1 << r)<v; r++);
	return r;
}

void* libmapping_matrix_malloc (uint32_t nrows, uint32_t ncols, uint32_t type_size)
{
	void **p;
	uint32_t i;
	
	p = (void**)lm_calloc(nrows, sizeof(void*));
	LM_ASSERT(p != NULL);
	
	p[0] = (void*)lm_calloc(nrows*ncols, type_size);
	LM_ASSERT(p[0] != NULL);
	for (i=1; i<nrows; i++)
		p[i] = p[0] + i*ncols*type_size;
	
	return (void*)p;
}

void libmapping_matrix_free (void *m)
{
	void **p = m;
	lm_free(p[0]);
	lm_free(p);
}


void libmapping_comm_matrix_init (comm_matrix_t *m, uint32_t nthreads)
{
#ifdef _SPCD
	m->matrix = (unsigned*) kmalloc (sizeof(unsigned) * nthreads * nthreads, GFP_KERNEL);
	LM_ASSERT(m->matrix != NULL);

#endif

	m->nthreads = nthreads;
}


void libmapping_comm_matrix_destroy (comm_matrix_t *m)
{
#ifdef _SPCD
	kfree(m->matrix);
#endif
}

#ifndef _SPCD
void libmapping_print_matrix (comm_matrix_t *m, FILE *fp)
{
	int i, j;
	for (i=m->nthreads-1; i>=0; i--) {
		for (j=0; j<m->nthreads; j++) {
			lm_fprintf(fp, PRINTF_UINT64, comm_matrix_ptr_el(m, i, j));
			if (j < m->nthreads-1)
				lm_fprintf(fp, ",");
		}
		lm_fprintf(fp, "\n");
	}
}

void libmapping_print_matrix_alive (comm_matrix_t *m, FILE *fp, struct thread_t **alive_threads)
{
	int i, j;
	for (i=m->nthreads-1; i>=0; i--) {
		for (j=0; j<m->nthreads; j++) {
			lm_fprintf(fp, PRINTF_UINT64, comm_matrix_ptr_el(m, i, j));
			if (j < m->nthreads-1)
				lm_fprintf(fp, ",");
		}
		lm_fprintf(fp, "\n");
	}
}

void libmapping_print_matrix_alive_fname (comm_matrix_t *m, char *fname, struct thread_t **alive_threads)
{
	FILE *fp;
	fp = fopen(fname, "w");
	if (fp) {
		libmapping_print_matrix_alive(m, fp, alive_threads);
		fclose(fp);
	}
}

void libmapping_set_aff_thread (pid_t pid, uint32_t cpu)
{
#if defined(linux) || defined (__linux)
	int ret;
	cpu_set_t mask;
	
	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);

	errno = 0;
	ret = sched_setaffinity(pid, sizeof(mask), &mask);
	
	//LM_ASSERT_PRINTF(!(ret == -1 && errno != 0), "cannot bind thread %u to PU %u\n", (uint32_t)pid, cpu)
	//LM_ASSERT_PRINTF(!(ret == -1), "cannot bind thread %u to PU %u\n", (uint32_t)pid, cpu)
#else
	#error Only linux is supported at the moment
#endif
}

char* libmapping_strtok(char *str, char *tok, char del, uint32_t bsize)
{
	char *p;
	uint32_t i;

	for (p=str, i=0; *p && *p != del; p++, i++) {
		LM_ASSERT(i < (bsize-1))
		*tok = *p;
		tok++;
	}

	*tok = 0;
	
	if (*p)
		return p + 1;
	else if (p != str)
		return p;
	else
		return NULL;
}
#endif /* !_SPCD */
