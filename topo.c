#include "spcd.h"
#include <linux/cpu.h>

#define for_each_sibling(s, cpu) for_each_cpu(s, cpu_sibling_mask(cpu))
 
void topo_init(void)
{
	unsigned long node, cpu, sibling;
	int curCPU = 0;
	// int nodes, processors, cores;

	printk("SPCD: detected hardware topology:\n");

	for_each_online_node(node) {
		int cpus[64] = {};
		int cores[64] = {};

		printk("  node: %lu\n", node);

		for_each_online_cpu(cpu) {
			int physID = cpu_data(cpu).phys_proc_id;
			if (cpu_to_node(cpu) != node)
				continue;
			if (!cpus[physID]) {
				printk("    processor: %d\n", physID);
				cpus[physID] = 1;
				curCPU = physID;
			}
			if (physID == curCPU && !cores[cpu]) {
				printk("      cores: %2lu", cpu);
				cores[cpu] = 1;
				for_each_sibling(sibling, cpu) {
					if (!cores[sibling]) {
						printk(", %2lu", sibling);
						cores[sibling] = 1;
					}
				}
				printk("\n");
			}
		}
	}
}

void topo_cleanup(void)
{
}
