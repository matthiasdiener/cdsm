#include "spcd.h"
#include <linux/cpu.h>

#define for_each_sibling(s, cpu) for_each_cpu(s, cpu_sibling_mask(cpu))

int nr_node;
int nr_cpu;
int nr_sibling;

enum tree_node_type {
	NUMA = 1,
	CPU,
	SIBLING,
};

struct tree_node {
	int id;
	struct tree_node *parent;
	struct tree_node *firstchild;
	struct tree_node *nextsibling;
};

struct tree_node *root;

void topo_init(void)
{
	unsigned long node;
	unsigned long cpu;
	unsigned long sibling;

	root = kmalloc(sizeof (struct tree_node), GFP_KERNEL);

	// for_each_online_node(node){
	// 	printk("node: %lu\n", node);
	// }

	for_each_present_cpu(cpu) {
		node = cpu_to_node(cpu);
		printk("cpu: %lu, node: %lu\n", cpu, node);
		for_each_sibling(sibling, cpu) {
			printk("\tsibling: %lu\n", sibling);
		}
	}
}



void topo_cleanup(void)
{
	kfree(root);
}