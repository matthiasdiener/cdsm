#include "spcd.h"

MODULE_LICENSE("GPL");

unsigned long pt_pf;
unsigned long pt_addr_conflict;
unsigned long pt_pf_extra;
unsigned long pt_num_walks;
unsigned long pt_pte_fixes;

static struct task_struct *pt_pf_thread;

int init_module(void)
{
	printk("Welcome.....\n");
	reset_stats();

	register_probes();

	pt_pf_thread = kthread_create(pt_pf_thread_func, NULL, "pt_pf_thread");
	wake_up_process(pt_pf_thread);

	return 0;
}


void cleanup_module(void)
{
	kthread_stop(pt_pf_thread);

	unregister_probes();

	printk("Bye.....\n");
}
