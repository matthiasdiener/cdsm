#include "spcd.h"


int spcd_check_name(char *name)
{
	const char *bm_names[] = {".x", /*NAS*/
	"blackscholes", "bodytrack", "facesim", "ferret", "freqmine", "rtview", "swaptions", "fluidanimate", "vips", "x264", "canneal", "dedup", "streamcluster", /*Parsec*/
	"LU","FFT", "CHOLESKY", /*Splash2*/
	};
	
	int i, len = sizeof(bm_names)/sizeof(bm_names[0]);

	if (pt_task)
		return 0;

	for (i=0; i<len; i++) {
		if (strstr(name, bm_names[i]))
			return 1;
	}
	return 0;
}

static struct jprobe spcd_pte_fault_jprobe = {
	.entry = spcd_pte_fault_handler,
	.kp.symbol_name = "handle_pte_fault",
};

static struct jprobe spcd_exit_process_probe = {
	.entry = spcd_exit_process_handler,
	.kp.symbol_name = "perf_event_exit_task",
};

static struct kretprobe spcd_new_process_probe = {
	.handler = spcd_new_process_handler,
	.kp.symbol_name = "do_execve",
};

static struct kretprobe spcd_fork_probe = {
	.handler = spcd_fork_handler,
	.kp.symbol_name = "do_fork",
};

static struct jprobe spcd_del_page_probe = {
	.entry = spcd_del_page_handler,
	.kp.symbol_name = "__delete_from_page_cache",
};

static struct jprobe spcd_unmap_page_range_probe = {
	.entry = spcd_unmap_page_range_handler,
	.kp.symbol_name = "unmap_page_range",
};


void register_probes(void)
{
	int ret;
	if ((ret=register_jprobe(&spcd_pte_fault_jprobe))) {
		printk("spcd_pte_fault_jprobe failed, %d\n", ret);
	}
	if ((ret=register_jprobe(&spcd_exit_process_probe))){
		printk("spcd_exit_process_probe failed, %d\n", ret);
	}
	if ((ret=register_kretprobe(&spcd_new_process_probe))){
		printk("spcd_new_process_probe failed, %d\n", ret);
	}
	if ((ret=register_kretprobe(&spcd_fork_probe))){
		printk("spcd_fork_probe failed, %d\n", ret);
	}
	if ((ret=register_jprobe(&spcd_del_page_probe))){
		printk("spcd_del_page_probe failed, %d\n", ret);
	}
	if ((ret=register_jprobe(&spcd_unmap_page_range_probe))){
		printk("spcd_zap_pte_range_probe failed, %d\n", ret);
	}
}

void unregister_probes(void)
{
	unregister_jprobe(&spcd_pte_fault_jprobe);
	unregister_jprobe(&spcd_exit_process_probe);
	unregister_kretprobe(&spcd_new_process_probe);
	unregister_kretprobe(&spcd_fork_probe);
	unregister_jprobe(&spcd_del_page_probe);
	unregister_jprobe(&spcd_unmap_page_range_probe);
}
