#include "spcd.h"

extern void map_simple(void);
extern void map_drake(int nt);

static long (*sched_setaffinity_p)(pid_t pid, const struct cpumask *in_mask);


int spcd_map_func(void* v)
{
	int nt, i;
	int arities[] = {num_nodes, num_cores, num_threads};
	int nlevels =  sizeof(arities)/sizeof(arities[0]);
	int npus = 0, nvertices = 0;
	int map[MAX_THREADS];
	topology_t *topo = libmapping_topology_get();

	libmapping_get_n_pus_fake_topology(arities, nlevels, &npus, &nvertices);
	printk("npus: %d, nvertices: %d\n", npus, nvertices);

	libmapping_graph_init(&topo->graph, nvertices, nvertices-1);

	topo->root = libmapping_create_fake_topology(arities, nlevels, pu);
	topo->root->weight = 0;
	topo->root->type = GRAPH_ELTYPE_ROOT;
	topo->pu_number = npus;

	libmapping_topology_print (topo);

	libmapping_mapping_algorithm_greedy_init(topo);

	// loop:
	while (1) {
		if (kthread_should_stop())
			break;
		
		nt = spcd_get_active_threads();
		if (nt >= 4) {
			// pt_print_share();
			// map_drake(nt);
			spcd_main_matrix.nthreads = nt;
			libmapping_mapping_algorithm_greedy_map (&spcd_main_matrix, map);
			printk("MAP ");
			for (i=0; i<nt; i++){
				printk("T%d->P%d ", i, map[i]);
			}
			printk("\n");
			//pt_share_clear();
		}
		msleep(1000);
	}
	
	libmapping_graph_destroy(&topo->graph);
	libmapping_mapping_algorithm_greedy_destroy();
	return 0;
}


void spcd_set_affinity(int tid, int core)
{
	int pid = pt_get_pid(tid);
	struct cpumask mask;
	long ret;

	cpumask_clear(&mask);
	cpumask_set_cpu(core, &mask);

	ret = (*sched_setaffinity_p)(pid, &mask);
	if (ret == 0)
		printk ("SPCD BUG: spcd_set_affinity failed\n");
}

void spcd_map_init(void)
{
	if (!sched_setaffinity_p) {
		sched_setaffinity_p = (void*) kallsyms_lookup_name("sched_setaffinity");
		if (!sched_setaffinity_p)
			printk("SPCD BUG: sched_setaffinity missing\n");
	}
}
