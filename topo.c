#include "spcd.h"
#include <linux/cpu.h>

#define for_each_sibling(s, cpu) for_each_cpu(s, cpu_sibling_mask(cpu))
#define for_each_core(s, cpu) for_each_cpu(s, cpu_core_mask(cpu))
#define for_each_node_cpu(s, node) for_each_cpu(s, cpumask_of_node(node))

int num_nodes = 0, num_cpus = 0, num_cores = 0, num_threads = 0;
int pu[256];

void topo_start(void)
{
	int node, cpu, sibling, core;
	int index = 0, i;
	int cpus[256] = {};

	printk("SPCD: detected hardware topology:\n");

	for_each_online_node(node) {
		printk("  node: %d\n", node);
		num_nodes++;

		for_each_node_cpu(cpu, node) {
			if (cpus[cpu])
				continue;
			printk("    processor: %d\n", cpu);
			num_cpus++;

			for_each_core(core, cpu) {
				if (cpus[core])
					continue;
				printk ("      core: %d", core);
				cpus[core] = 1;
				num_cores++; num_threads++; pu[index++] = core;
				for_each_sibling(sibling, core) {
					if (cpus[sibling])
						continue;
					cpus[sibling] = 1;
					num_threads++;
					printk(", %d", sibling);
					pu[index++] = sibling;
				}
				printk("\n");
			}
		}
	}

	printk("PU: ");
	for (i=0; i<num_threads; i++) {
		printk("%d ", pu[i]);
	}

	// if (!num_cores)
	// 	num_cores++;

	num_threads /= num_cores;
	num_cores /= num_cpus;
	num_cpus /= num_nodes;

	printk("\nSPCD: %d nodes, %d processors per node, %d cores per processor, %d threads per core\n", num_nodes, num_cpus, num_cores, num_threads);
}

void topo_stop(void)
{
}
