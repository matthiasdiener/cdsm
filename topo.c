#include "spcd.h"
#include <linux/cpu.h>

#define for_each_sibling(s, cpu) for_each_cpu(s, cpu_sibling_mask(cpu))

static int nodes[16];
static int cpus[16];
// static int cores[64];


void topo_init(void)
{
	unsigned long node;
	unsigned long cpu;
	int curCPU = 0;
	// unsigned long sibling;

	for_each_online_node(node){
		nodes[node] = 1;
		printk("node: %lu\n", node);
		for_each_present_cpu(cpu) {
			int physID = cpu_data(cpu).phys_proc_id;
			if (!cpus[physID]) {
				printk("  processor: %d\n", physID);
				cpus[physID] = 1;
				curCPU = physID;
			}
			if (physID == curCPU)
				printk("    core: %lu\n", cpu);
		}
	}

	// for_each_present_cpu(cpu) {
	// 	node = cpu_to_node(cpu);
	// 	printk("cpu: %lu, node: %lu\n", cpu, node);
	// 	for_each_sibling(sibling, cpu) {
	// 		printk("\tsibling: %lu\n", sibling);
	// 	}
	// }
}



void topo_cleanup(void)
{
}
