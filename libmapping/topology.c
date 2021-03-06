#ifdef LIBMAPPING_SUPPORT_HWLOC
	#ifndef _GNU_SOURCE
		#define _GNU_SOURCE
	#endif
	#include <hwloc.h>
#endif

#include "libmapping.h"

struct topology_walk_tmp_t {
	topology_t *t;
	uint32_t i, j;
};

topology_t libmapping_topology;

// too lazy to replace topology to libmapping_topology
#define topology libmapping_topology

#define dist(i, j) dist_[ ((i) << dist_dim_log) + (j) ]
#define dist_pus(i, j) t->dist_pus_[ ((i) << t->dist_pus_dim_log) + (j) ]

static void floyd_warshall (topology_t *t)
{
	uint32_t i, j, k;
	uint32_t *dist_;
	uint32_t dist_dim;
	uint32_t dist_dim_log;

	dist_dim = libmapping_get_next_power_of_two(t->graph.n_vertices);
	dist_dim_log = libmapping_get_log2(dist_dim);
	t->dist_pus_dim = libmapping_get_next_power_of_two(t->pu_number);
	t->dist_pus_dim_log = libmapping_get_log2(t->dist_pus_dim);

	dprintf("topology n_vertices %u dist_dim %u dist_dim_log %u const_max %u dist_pus_dim %u dist_pus_dim_log %u\n", t->graph.n_vertices, dist_dim, dist_dim_log, USHRT_MAX, t->dist_pus_dim, t->dist_pus_dim_log);

	dist_ = (uint32_t*)lm_calloc(dist_dim*dist_dim, sizeof(uint32_t));
	LM_ASSERT(dist_ != NULL);
	t->dist_pus_ = (uint32_t*)lm_calloc(t->dist_pus_dim*t->dist_pus_dim, sizeof(uint32_t));
	LM_ASSERT(t->dist_pus_ != NULL);

	for (i=0; i<t->graph.n_vertices; i++) {
		dist(i, i) = 0;
		for (j=i+1; j<t->graph.n_vertices; j++) {
			dist(i, j) = dist(j, i) = USHRT_MAX; //UINT_MAX;
		}
	}

	for (i=0; i<t->graph.n_vertices; i++) {
		for (j=0; j<t->graph.vertices[i].arity; j++) {
			dist(i, t->graph.vertices[i].linked[j].v->pos) = t->graph.vertices[i].linked[j].e->weight;
		}
	}

	for (k=0; k<t->graph.n_vertices; k++) {
		for (i=0; i<t->graph.n_vertices; i++) {
			for (j=0; j<t->graph.n_vertices; j++) {
				uint32_t sum;
				sum = dist(i, k) + dist(k, j);
				//printf("sum is %llu\n", sum);
				if (dist(i, j) > sum) {
					dist(i, j) = sum;
				}
			}
		}
	}

	for (i=0; i<t->pu_number; i++) {
		for (j=0; j<t->pu_number; j++) {
			dist_pus(i, j) = 0;
		}
	}

	for (i=0; i<t->graph.n_vertices; i++) {
		for (j=0; j<t->graph.n_vertices; j++) {
			if (t->graph.vertices[i].type == GRAPH_ELTYPE_PU && t->graph.vertices[j].type == GRAPH_ELTYPE_PU) {
				dist_pus(t->graph.vertices[i].id, t->graph.vertices[j].id) = dist(i, j);
			}
		}
	}

/*	for (i=0; i<t->pu_number; i++) {*/
/*		for (j=0; j<t->pu_number; j++) {*/
/*			if (i != j) {*/
/*				LM_ASSERT(dist_pus(i, j) > 0);*/
/*			}*/
/*			else {*/
/*				LM_ASSERT(dist_pus(i, j) == 0);*/
/*			}*/
/*		}*/
/*	}*/

#if defined(DEBUG) && 0
	for (i=0; i<t->pu_number; i++) {
		dprintf("distance pu %u:\n", i);
		for (j=0; j<t->pu_number; j++) {
			dprintf("\t-> %u:   %u\n", j, dist_pus(i, j));
		}
	}
#endif

	lm_free(dist_);
}

static int topology_walk_pre_order (libmapping_topology_walk_routine_t routine, vertex_t *v, vertex_t *from, edge_t *edge, void *data, uint32_t level)
{
	if (routine(data, v, from, edge, level)) {
		uint32_t i;

		for (i=0; i<v->arity; i++) {
			if (v->linked[i].v != from) {
				if (!topology_walk_pre_order(routine, v->linked[i].v, v, v->linked[i].e, data, level + 1))
					return 0;
			}
		}

		return 1;
	}
	else
		return 0;
}

void libmapping_topology_walk_pre_order (topology_t *topology, libmapping_topology_walk_routine_t routine, void *data)
{
	topology_walk_pre_order(routine, topology->root, NULL, NULL, data, 0);
}

topology_t* libmapping_topology_get ()
{
	return &topology;
}

/*static void topology_walk_pos_order (libmapping_topology_walk_routine_t routine, vertex_t *v, vertex_t *from, edge_t *edge, void *data, uint32_t level)*/
/*{*/

/*}*/

/*void libmapping_topology_walk_pos_order (libmapping_topology_walk_routine_t routine, void *data)*/
/*{*/
/*	*/
/*}*/

void libmapping_get_n_pus_fake_topology (uint32_t *arities, uint32_t nlevels, uint32_t *npus, uint32_t *nvertices)
{
	int j;
	if (nlevels == 0) {
		(*npus)++;
		(*nvertices)++;
	}
	else {
		(*nvertices)++;
		for (j=0; j < *arities; j++) {
			libmapping_get_n_pus_fake_topology(arities+1, nlevels-1, npus, nvertices);
		}
	}
}

static vertex_t* create_fake_topology (uint32_t level, uint32_t *arities, uint32_t nlevels, uint32_t *pus, weight_t *weights, uint32_t numa_node)
{
	int j;
	vertex_t *v, *link;
	edge_t *e;
/*printf("level %u with %u numa %u\n", level, *arities, numa_node);*/
	v = libmapping_get_free_vertex(&topology.graph);

	if (nlevels == 0) {
		static uint32_t id = 0;
		v->type = GRAPH_ELTYPE_PU;
		if (pus != NULL)
			v->id = pus[id];
		else
			v->id = id;
		id++;
	}
	else {
		if (numa_node && level == 1) {
			static uint32_t id = 0;
			v->type = GRAPH_ELTYPE_NUMA_NODE;
			v->id = id++;
		}
		else {
			v->type = GRAPH_ELTYPE_UNDEFINED;
			v->id = 0;
		}
		for (j=0; j < *arities; j++) {
			link = create_fake_topology(level+1, arities+1, nlevels-1, pus, (weights == NULL) ? NULL : weights+1, numa_node);
			e = libmapping_graph_connect_vertices(&topology.graph, v, link);
			if (weights == NULL)
				e->weight = 1 << (nlevels);
			else
				e->weight = *weights;
			e->type = GRAPH_ELTYPE_UNDEFINED;
		}
	}

	return v;
}

vertex_t* libmapping_create_fake_topology (uint32_t *arities, uint32_t nlevels, uint32_t *pus, weight_t *weights)
{
	return create_fake_topology(0, arities, nlevels, pus, weights, 0);
}

static int print_topology (void *d, vertex_t *v, vertex_t *from, edge_t *edge, uint32_t level)
{
	uint32_t i;

	lm_fprintf((FILE*)d, PRINTF_PREFIX);
	for (i=0; i < level; i++)
		lm_fprintf((FILE*)d, "\t");
	lm_fprintf((FILE*)d, "%s (id: %u, arity: %u)\n", libmapping_graph_eltype_str(v->type), v->id, (v->type == GRAPH_ELTYPE_ROOT) ? v->arity : v->arity - 1);

	return 1;
}

#ifdef _SPCD
void libmapping_topology_print (topology_t *t)
{
	libmapping_topology_walk_pre_order(t, print_topology, NULL);
}
#else
void libmapping_topology_print (topology_t *t, FILE *out)
{
	libmapping_topology_walk_pre_order(t, print_topology, stdout);
}
#endif

#if defined(LIBMAPPING_SUPPORT_HWLOC)

static uint32_t lm_hwloc_get_n_vertices (hwloc_topology_t hwloc_topology, hwloc_obj_t obj)
{
	uint32_t i, n;
	n = 1;
	for (i=0; i<obj->arity; i++)
		n += lm_hwloc_get_n_vertices(hwloc_topology, obj->children[i]);
	return n;
}

static vertex_t* lm_hwloc_load_topology (hwloc_topology_t hwloc_topology, hwloc_obj_t obj)
{
	uint32_t i;
	vertex_t *v, *link;
	edge_t *e;
	weight_t eweight;

/*		{*/
/*    char string[128];*/
/*    hwloc_obj_snprintf(string, sizeof(string), hwloc_topology, obj, "#", 0);*/
/*    printf("%s\n", string);*/
/*    }*/

#if 0
	if (obj->type != HWLOC_OBJ_MACHINE && obj->type != HWLOC_OBJ_PU && obj->arity == 1) {
		return lm_hwloc_load_topology(hwloc_topology, obj->children[0]);
	}
#endif

	v = libmapping_get_free_vertex(&topology.graph);
	eweight = 1;

	switch (obj->type) {
		case HWLOC_OBJ_MACHINE:
			v->type = GRAPH_ELTYPE_ROOT;
			break;

		case HWLOC_OBJ_NODE:
			v->type = GRAPH_ELTYPE_NUMA_NODE;
			eweight = 100;
			break;

		case HWLOC_OBJ_SOCKET:
			v->type = GRAPH_ELTYPE_SOCKET;
			break;

		case HWLOC_OBJ_CACHE:
			v->type = GRAPH_ELTYPE_CACHE;
			break;

		case HWLOC_OBJ_CORE:
			v->type = GRAPH_ELTYPE_CORE;
			eweight = 0;
			break;

		case HWLOC_OBJ_PU:
			v->type = GRAPH_ELTYPE_PU;
			break;

		default:
			v->type = GRAPH_ELTYPE_UNDEFINED;
	}

	v->weight = 0;
	v->id = obj->os_index;

	for (i=0; i<obj->arity; i++) {
		link = lm_hwloc_load_topology(hwloc_topology, obj->children[i]);
		e = libmapping_graph_connect_vertices(&topology.graph, v, link);
		e->weight = eweight;
		e->type = GRAPH_ELTYPE_UNDEFINED;
	}

	return v;
}

#endif

static vertex_t* optimize_topology_ (vertex_t *obj, vertex_t *from, weight_t ac_weight, topology_t *opt, uint32_t level)
{
	uint32_t i;
	vertex_t *v, *link;
	edge_t *e;

/*		{*/
/*    char string[128];*/
/*    hwloc_obj_snprintf(string, sizeof(string), hwloc_topology, obj, "#", 0);*/
/*    printf("%s\n", string);*/
/*    }*/

	if (obj->type != GRAPH_ELTYPE_ROOT && obj->type != GRAPH_ELTYPE_PU) {
		LM_ASSERT(obj->arity > 1)
		if (obj->arity == 2) {
/*			dprintf("%*sskipping level %s arity %u\n", level, "", libmapping_graph_eltype_str(obj->type), obj->arity);*/
			if (obj->linked[0].v != from)
				return optimize_topology_(obj->linked[0].v, obj, ac_weight+obj->linked[0].e->weight, opt, level+1);
			else
				return optimize_topology_(obj->linked[1].v, obj, ac_weight+obj->linked[1].e->weight, opt, level+1);
		}
	}

	v = libmapping_get_free_vertex(&opt->graph);

	v->weight = 0;
	v->id = obj->id;
	v->type = obj->type;

/*	dprintf("%*sadding level %s arity %u id %u\n", level, "", libmapping_graph_eltype_str(obj->type), obj->arity, obj->id);*/

	for (i=0; i<obj->arity; i++) {
		if (obj->linked[i].v != from) {
			link = optimize_topology_(obj->linked[i].v, obj, 0, opt, level+1);
			e = libmapping_graph_connect_vertices(&opt->graph, v, link);
			e->weight = ac_weight + obj->linked[i].e->weight;
			e->type = GRAPH_ELTYPE_UNDEFINED;
		}
	}

	return v;
}

static void optimize_topology (topology_t *t, topology_t *opt)
{
	libmapping_graph_init(&opt->graph, t->graph.n_vertices, t->graph.n_vertices - 1);
	opt->root = optimize_topology_(t->root, NULL, 0, opt, 0);
}

static int get_topology_attr_pu_number (void *d, vertex_t *v, vertex_t *from, edge_t *edge, uint32_t level)
{
	topology_t *t = (topology_t*)d;
	if (v->type == GRAPH_ELTYPE_PU)
		t->pu_number++;
	return 1;
}

static int detect_n_levels (void *d, vertex_t *v, vertex_t *previous_vertex, edge_t *edge, uint32_t level)
{
	topology_t *t = (topology_t*)d;
	t->n_levels++;
	if (v->type == GRAPH_ELTYPE_PU)
		return 0;
	else
		return 1;
}

static int get_topology_best_pus_number (void *d, vertex_t *v, vertex_t *from, edge_t *edge, uint32_t level)
{
	struct topology_walk_tmp_t *stc = (struct topology_walk_tmp_t*)d;
	if (v->type == GRAPH_ELTYPE_PU) {
		stc->t->best_pus[stc->i] = v->id;
		stc->i++;
	}
	return 1;
}

static int detect_arity_of_levels (void *d, vertex_t *v, vertex_t *previous_vertex, edge_t *edge, uint32_t level)
{
	struct topology_walk_tmp_t *stc = (struct topology_walk_tmp_t*)d;

	if (v->type == GRAPH_ELTYPE_ROOT) {
		stc->t->arities[stc->i] = v->arity;
	}
	else {
		stc->t->arities[stc->i] = v->arity - 1;
	}

	stc->i++;

	if (v->type == GRAPH_ELTYPE_PU)
		return 0;
	else
		return 1;
}

static int detect_link_weight_of_levels (void *d, vertex_t *v, vertex_t *previous_vertex, edge_t *edge, uint32_t level)
{
	struct topology_walk_tmp_t *stc = (struct topology_walk_tmp_t*)d;

	if (v->type == GRAPH_ELTYPE_ROOT)
		return 1;

	stc->t->link_weights[stc->i] = edge->weight;
	stc->i++;

	if (v->type == GRAPH_ELTYPE_PU)
		return 0;
	else
		return 1;
}

static int get_n_numa_nodes (void *d, vertex_t *v, vertex_t *from, edge_t *edge, uint32_t level)
{
	struct topology_walk_tmp_t *stc = (struct topology_walk_tmp_t*)d;
	if (v->type == GRAPH_ELTYPE_NUMA_NODE) {
		stc->t->n_numa_nodes++;
	}
	return 1;
}

static int get_pus_of_numa_nodes (void *d, vertex_t *v, vertex_t *from, edge_t *edge, uint32_t level)
{
	struct topology_walk_tmp_t *stc = (struct topology_walk_tmp_t*)d;
	if (v->type == GRAPH_ELTYPE_NUMA_NODE) {
		stc->i = v->id;
		stc->j = 0;
	}
	else if (v->type == GRAPH_ELTYPE_PU) {
		LM_ASSERT(stc->i < stc->t->n_numa_nodes && stc->j < stc->t->n_pus_per_numa_node)
		stc->t->pus_of_numa_node[stc->i][stc->j] = v->id;
		stc->j++;
	}
	return 1;
}

static void topology_analysis_ (topology_t *t, topology_t *orig)
{
	struct topology_walk_tmp_t tmp;
	int32_t i, j;

#ifdef DEBUG
	libmapping_topology_print(t, LM_PRINTF_OUT);
#endif

	tmp.t = t;

	t->pu_number = 0;
	libmapping_topology_walk_pre_order(t, get_topology_attr_pu_number, t);

	t->n_levels = 0;
	libmapping_topology_walk_pre_order(t, detect_n_levels, t);

	if (orig == NULL) {
		t->best_pus = (uint32_t*)lm_calloc(t->pu_number, sizeof(uint32_t));
		LM_ASSERT( t->best_pus != NULL );

		tmp.i = 0;
		libmapping_topology_walk_pre_order(t, get_topology_best_pus_number, &tmp);
	}
	else {
		t->best_pus = orig->best_pus;
	}

	if (orig == NULL) {
	#ifndef _SPCD
		t->page_size = sysconf(_SC_PAGESIZE);
	#else
		t->page_size = 1 << PAGE_SHIFT;
	#endif
		t->page_shift = libmapping_get_log2(t->page_size);
		t->offset_addr_mask = t->page_size - 1;
		t->page_addr_mask = ~t->offset_addr_mask;
	}
	else {
		t->page_size = orig->page_size;
		t->page_shift = orig->page_shift;
		t->offset_addr_mask = orig->offset_addr_mask;
		t->page_addr_mask = orig->page_addr_mask;
	}

	t->arities = (uint32_t*)lm_calloc(t->n_levels, sizeof(uint32_t));
	LM_ASSERT(t->arities != NULL)

	t->link_weights = (uint32_t*)lm_calloc(t->n_levels, sizeof(uint32_t));
	LM_ASSERT(t->link_weights != NULL)

	t->nobjs_per_level = (uint32_t*)lm_calloc(t->n_levels, sizeof(uint32_t));
	LM_ASSERT(t->nobjs_per_level != NULL)

	tmp.i = 0;
	libmapping_topology_walk_pre_order(t, detect_arity_of_levels, &tmp);

	tmp.i = 0;
	libmapping_topology_walk_pre_order(t, detect_link_weight_of_levels, &tmp);

	t->nobjs_per_level[t->n_levels-1] = t->pu_number;
	for (i=t->n_levels-2; i>=0; i--)
		t->nobjs_per_level[i] = t->nobjs_per_level[i+1] / t->arities[i];

	if (orig == NULL) {
		t->n_numa_nodes = 0;
		libmapping_topology_walk_pre_order(t, get_n_numa_nodes, &tmp);

		dprintf("%u numa nodes\n", t->n_numa_nodes);

		if (t->n_numa_nodes > 0) {
			t->n_pus_per_numa_node = t->pu_number / t->n_numa_nodes;
			dprintf("%u pus per numa nodes\n", t->n_pus_per_numa_node);
			t->pus_of_numa_node = libmapping_matrix_malloc(t->n_numa_nodes, t->n_pus_per_numa_node, sizeof(uint32_t));
			libmapping_topology_walk_pre_order(t, get_pus_of_numa_nodes, &tmp);
		}
		else {
			t->n_numa_nodes = 1;
			t->n_pus_per_numa_node = t->pu_number;
			t->pus_of_numa_node = libmapping_matrix_malloc(t->n_numa_nodes, t->n_pus_per_numa_node, sizeof(uint32_t));

			for (i=0; i<t->pu_number; i++)
				t->pus_of_numa_node[0][i] = t->best_pus[i];
		}

		t->pus_to_numa_node = (uint32_t*)lm_calloc(t->pu_number, sizeof(uint32_t));
		LM_ASSERT(t->pus_to_numa_node != NULL)

		for (i=0; i<t->pu_number; i++)
			t->pus_to_numa_node[i] = 0xFFFFFFFF;

		for (i=0; i<t->n_numa_nodes; i++) {
			for (j=0; j<t->n_pus_per_numa_node; j++)
				t->pus_to_numa_node[ t->pus_of_numa_node[i][j] ] = i;
		}

		for (i=0; i<t->pu_number; i++) {
			LM_ASSERT_PRINTF(t->pus_to_numa_node[i] != 0xFFFFFFFF, "i=%u\n", i);
		}
	}
	else {
		t->n_numa_nodes = orig->n_numa_nodes;
		t->n_pus_per_numa_node = orig->n_pus_per_numa_node;
		t->pus_of_numa_node = orig->pus_of_numa_node;
		t->pus_to_numa_node = orig->pus_to_numa_node;
	}

	floyd_warshall(t);

#ifdef DEBUG
{
	uint32_t i, j;

	dprintf("detected topology: (%u pu, %u levels, %u numa nodes, %u page size, %u page shift)\n", t->pu_number, t->n_levels, t->n_numa_nodes, t->page_size, t->page_shift);

	dprintf("best pus: ");
	for (i=0; i<t->pu_number; i++) {
		lm_printf("%u,", t->best_pus[i]);
	}
	lm_printf("\n");

	dprintf("arities: ");
	for (i=0; i<t->n_levels; i++) {
		lm_printf("%u,", t->arities[i]);
	}
	lm_printf("\n");

	dprintf("link weights: ");
	for (i=0; i<t->n_levels; i++) {
		lm_printf("%u,", t->link_weights[i]);
	}
	lm_printf("\n");

	for (i=0; i<t->n_numa_nodes; i++) {
		dprintf("pus in numa node %u: ", i);
		for (j=0; j<t->n_pus_per_numa_node; j++) {
			lm_printf("%u,", t->pus_of_numa_node[i][j]);
		}
		lm_printf("\n");
	}
	lm_printf("\n");

	dprintf("pus to numa node: ");
	for (i=0; i<t->pu_number; i++) {
		lm_printf("%u,", t->pus_to_numa_node[i]);
	}
	lm_printf("\n");
}
#endif
}

void libmapping_topology_analysis (topology_t *t)
{
	t->opt_topology = (topology_t*)lm_calloc(1, sizeof(topology_t));
	LM_ASSERT(t->opt_topology != NULL)
	t->opt_topology->opt_topology = t->opt_topology;

	LM_ASSERT( t->root->type == GRAPH_ELTYPE_ROOT );
	LM_ASSERT( t->graph.n_vertices == t->graph.used_vertices );
	LM_ASSERT( t->graph.n_edges == t->graph.used_edges );

	topology_analysis_(t, NULL);
	return;

	optimize_topology(t, t->opt_topology);

	LM_ASSERT( t->opt_topology->root->type == GRAPH_ELTYPE_ROOT );
	LM_ASSERT( t->opt_topology->graph.n_vertices >= t->opt_topology->graph.used_vertices );
	LM_ASSERT( t->opt_topology->graph.n_edges >= t->opt_topology->graph.used_edges );

	topology_analysis_(t->opt_topology, t);
}

#ifndef _SPCD

void libmapping_topology_init()
{
	if (libmapping_env_get_str((char*)libmapping_envname(ENV_LIBMAPPING_TOPOLOGY_ARITIES))) {
		uint32_t arities[64], nlevels, *pus = NULL; // if there are more than 64 levels, well...
		uint32_t npus = 0, nvertices = 0, numa = 0;
		weight_t *weights = NULL;

		nlevels = libmapping_env_to_vector((char*)libmapping_envname(ENV_LIBMAPPING_TOPOLOGY_ARITIES), (int32_t*)arities, 64, 1);
		LM_ASSERT_PRINTF(nlevels > 0, "Number of levels defined in env %s must be higher than 1\n", libmapping_envname(ENV_LIBMAPPING_TOPOLOGY_ARITIES));

		libmapping_get_n_pus_fake_topology(arities, nlevels, &npus, &nvertices);
		LM_ASSERT_PRINTF(npus > 0, "Number of PUs can't be %u\n", npus);

		if (libmapping_env_get_str((char*)libmapping_envname(ENV_LIBMAPPING_TOPOLOGY_PU_IDS))) {
			pus = (uint32_t*)lm_calloc(npus, sizeof(uint32_t));
			LM_ASSERT(pus != NULL)

			libmapping_env_to_vector((char*)libmapping_envname(ENV_LIBMAPPING_TOPOLOGY_PU_IDS), (int32_t*)pus, npus, 0);
		}

		libmapping_env_get_integer((char*)libmapping_envname(ENV_LIBMAPPING_TOPOLOGY_NUMA), (int32_t*)&numa);

		dprintf("topology from env var: there are %u pus, %u vertices, %u levels, %u numa\n", npus, nvertices, nlevels, numa);

		libmapping_graph_init(&topology.graph, nvertices, nvertices-1);
		topology.root = create_fake_topology(0, arities, nlevels, pus, weights, numa);
		topology.root->weight = 0;
		topology.root->type = GRAPH_ELTYPE_ROOT;

		if (pus != NULL)
			lm_free(pus);
	}
	else { // detect automatically
	#if defined(LIBMAPPING_SUPPORT_HWLOC)
		hwloc_topology_t hwloc_topology;
		uint32_t n_vertices;

		hwloc_topology_init(&hwloc_topology);
/*		hwloc_topology_ignore_all_keep_structure(&hwloc_topology);*/
		hwloc_topology_load(hwloc_topology);

		n_vertices = lm_hwloc_get_n_vertices(hwloc_topology, hwloc_get_root_obj(hwloc_topology));
		libmapping_graph_init(&topology.graph, n_vertices, n_vertices - 1);

		dprintf("Hwloc (%u vertices)\n", n_vertices);
		topology.root = lm_hwloc_load_topology(hwloc_topology, hwloc_get_root_obj(hwloc_topology));
	#elif defined(_SC_NPROCESSORS_ONLN)
		uint32_t npus, arities[1];

		npus = sysconf(_SC_NPROCESSORS_ONLN);
		dprintf("No Hwloc, using fake topology (%u pus)\n", npus);

		libmapping_graph_init(&topology.graph, npus+1, npus);
		arities[0] = npus;
		topology.root = libmapping_create_fake_topology(arities, 1, NULL, NULL);
		topology.root->weight = 0;
		topology.root->type = GRAPH_ELTYPE_ROOT;
	#else
		#error There is no way to detect the number of cpus
	#endif
	}

/*	LM_ASSERT( topology.root->type == GRAPH_ELTYPE_ROOT );*/
/*	LM_ASSERT( topology.graph.n_vertices == topology.graph.used_vertices );*/
/*	LM_ASSERT( topology.graph.n_edges == topology.graph.used_edges );*/

	libmapping_topology_analysis(&topology);
}

void libmapping_topology_destroy (void)
{
	libmapping_graph_destroy(&topology.graph);
	lm_free(topology.dist_pus_);
	lm_free(topology.best_pus);
}

#endif /* !_SPCD */

