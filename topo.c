#include "spcd.h"
#include <linux/cpu.h>

#define for_each_sibling(i, cpu) for_each_cpu(i, cpu_sibling_mask(cpu))

void topo_init(void)
{
	unsigned long cpu;
	unsigned long node;
	unsigned long sibling;

	for_each_online_node(node){
		printk("node: %lu\n", node);
	}

	for_each_present_cpu(cpu) {
		printk("cpu: %lu, node: %d\n", cpu, cpu_to_node(cpu));
		for_each_sibling(sibling, cpu) {
			printk("\tsibling: %lu\n", sibling);
		}
	}
}