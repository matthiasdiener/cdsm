#ifndef __LIBMAPPING_LIB_H__
#define __LIBMAPPING_LIB_H__

#ifndef _SPCD
	#define LM_PRINTF_OUT stdout
	#define lm_printf(...) fprintf(LM_PRINTF_OUT, __VA_ARGS__)
	#define lm_fprintf(out, ...) fprintf(out, __VA_ARGS__)

	#ifdef LIBMAPPING_DATA_MAPPING_MALLOC
		void* lm_calloc (size_t count, size_t tsize);
		void lm_free (void *p);
	#else
		#define lm_calloc(count, tsize) calloc((count), (tsize))
		#define lm_free(p) free(p)
	#endif
#else
	#define LM_PRINTF_OUT NULL
	#define lm_printf(...) printk(__VA_ARGS__)
	#define lm_fprintf(out, ...) printk(__VA_ARGS__)
	
	#define lm_calloc(count, tsize) kmalloc((count) * (tsize), GFP_KERNEL)
	#define lm_free(p) kfree(p)
#endif

#define LM_ASSERT(V) LM_ASSERT_PRINTF(V, "bye!\n")

#define LM_ASSERT_PRINTF(V, ...) \
	if (unlikely(!(V))) { \
		lm_printf(PRINTF_PREFIX "sanity error!\nfile %s at line %u assertion failed!\n%s\n", __FILE__, __LINE__, #V); \
		lm_printf(__VA_ARGS__); \
		libmapping_panic(1); \
	}

#ifdef DEBUG
	#define dprintf(...) lm_printf(PRINTF_PREFIX __VA_ARGS__)
#else
	#define dprintf(...)
#endif


#ifndef _SPCD
	#define likely(x)       __builtin_expect((x),1)
	#define unlikely(x)     __builtin_expect((x),0)
#endif

#ifndef _SPCD
	struct thread_t;
	typedef struct comm_matrix_t {
	//	uint64_t **values;
		uint64_t values_[MAX_THREADS][MAX_THREADS];
		uint32_t nthreads;
	} comm_matrix_t;

	static inline uint64_t comm_matrix_ptr_el (comm_matrix_t *m, uint32_t row, uint32_t col)
	{
		return (row < col) ? m->values_[row][col] : m->values_[col][row];
	}
	
	#define comm_matrix_el(m, row, col)  comm_matrix_ptr_el(&(m), row, col)
	
	static inline void comm_matrix_ptr_write (comm_matrix_t *m, uint32_t row, uint32_t col, uint64_t val)
	{
		if (row < col)
			m->values_[row][col] = val;
		else
			m->values_[col][row] = val;
	}
	
	#define comm_matrix_write(m, row, col, val)  comm_matrix_ptr_write(&(m), row, col, val)
	
	static inline void comm_matrix_ptr_add (comm_matrix_t *m, uint32_t row, uint32_t col, uint64_t val)
	{
		if (row < col)
			m->values_[row][col] += val;
		else
			m->values_[col][row] += val;
	}
	
	#define comm_matrix_add(m, row, col, val)  comm_matrix_ptr_add(&(m), row, col, val)
	
	static inline void comm_matrix_ptr_sub (comm_matrix_t *m, uint32_t row, uint32_t col, uint64_t val)
	{
		if (row < col)
			m->values_[row][col] -= val;
		else
			m->values_[col][row] -= val;
	}
	
	#define comm_matrix_sub(m, row, col, val)  comm_matrix_ptr_sub(&(m), row, col, val)
	
#else
	struct spcd_share_matrix {
		unsigned *matrix;
		spinlock_t lock;
		uint32_t nthreads;
	};
	typedef struct spcd_share_matrix comm_matrix_t;
	extern int max_threads_bits;

	static inline
	unsigned get_matrix(struct spcd_share_matrix *m, int i, int j)
	{
		return i > j ? m->matrix[(i<<max_threads_bits) + j] : m->matrix[(j<<max_threads_bits) + i];
	}

	static inline
	void set_matrix(struct spcd_share_matrix *m, int i, int j, unsigned v)
	{
		if (i > j)
			m->matrix[(i << max_threads_bits) + j] = v;
		else
			m->matrix[(j << max_threads_bits) + i] = v;
	}


	#define comm_matrix_el(m, row, col) get_matrix(&(m), row, col)
	#define comm_matrix_ptr_el(m, row, col) get_matrix(m, row, col)
	#define comm_matrix_ptr_write(m, row, col, v) set_matrix(m, row, col, v)
#endif

void* libmapping_matrix_malloc (uint32_t nrows, uint32_t ncols, uint32_t type_size);
void libmapping_matrix_free (void *m);
void libmapping_comm_matrix_init (comm_matrix_t *m, uint32_t nthreads);
void libmapping_comm_matrix_destroy (comm_matrix_t *m);
#ifndef _SPCD
	void libmapping_print_matrix (comm_matrix_t *m, FILE *fp);
	void libmapping_print_matrix_alive (comm_matrix_t *m, FILE *fp, struct thread_t **alive_threads);
	void libmapping_print_matrix_alive_fname (comm_matrix_t *m, char *fname, struct thread_t **alive_threads);
#endif
void libmapping_set_aff_thread (pid_t pid, uint32_t cpu);
char* libmapping_strtok(char *str, char *tok, char del, uint32_t bsize);

static inline uint64_t libmapping_get_high_res_time(void)
{
	uint64_t r;
#if defined(__i386) || defined(__x86_64__)
	uint32_t lo, hi;
	__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
	r = ((uint64_t)hi << 32) | lo;
#elif defined(__ia64)
	__asm__ __volatile__ ("mov %0=ar.itc" : "=r" (r) :: "memory");
	//while (__builtin_expect ((int) result == -1, 0))
	//__asm__ __volatile__("mov %0=ar.itc" : "=r"(result) ::"memory");
#else
	#error Architecture does not support high resolution counter
#endif
	return r;
}

uint64_t libmapping_get_next_power_of_two (uint64_t v);
uint8_t libmapping_get_log2 (uint64_t v);

#endif
