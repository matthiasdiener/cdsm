#ifndef __LIBMAPPING_MAPPING_ALGORITHMS_H__
#define __LIBMAPPING_MAPPING_ALGORITHMS_H__

typedef struct thread_map_alg_init_t {
	topology_t *topology;
} thread_map_alg_init_t;

typedef struct thread_map_alg_map_t {
	// input
	void *foo;
	comm_matrix_t *m_init;
	thread_t **alive_threads;
	
	// output
	uint32_t *map;
} thread_map_alg_map_t;

extern void (*libmapping_mapping_algorithm_map) (thread_map_alg_map_t*);
extern void (*libmapping_mapping_algorithm_destroy) (void*);

void* libmapping_mapping_algorithm_setup(topology_t *topology, char *alg);

#ifdef LM_TMAP
#error LM_TMAP already defined
#endif
#define LM_TMAP(label, fini, fmap, fdestroy) \
	void* fini (thread_map_alg_init_t *data);\
	void fmap (thread_map_alg_map_t *data);\
	void fdestroy (void *data);
#include "mapping-algorithms-list.h"
#undef LM_TMAP

#endif
