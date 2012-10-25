#include "pt_comm.h"

MODULE_LICENSE("GPL");

unsigned long pt_pf;
unsigned long pt_addr_conflict;
unsigned long pt_pf_extra;
unsigned pt_num_faults;
int pt_num_threads;
unsigned long pt_num_walks;

struct task_struct *pt_task;

unsigned pt_num_faults = 3;
unsigned long pt_pte_fixes;

unsigned long pt_next_addr;
struct vm_area_struct *pt_next_vma;

unsigned long share [PT_MAXTHREADS][PT_MAXTHREADS];

struct task_struct *pt_thread;

DEFINE_SPINLOCK(ptl);

DEFINE_SPINLOCK(ptl2);

int spcd_check_name(char *name)
{
	const char *bm_names[] = {".x", /*NAS*/
	"blackscholes", "bodytrack", "facesim", "ferret", "freqmine", "rtview", "swaptions", "fluidanimate", "vips", "x264", "canneal", "dedup", "streamcluster", /*Parsec*/
	"LU","FFT", "CHOLESKY" /*Splash2*/
	};
	
	int i, len = sizeof(bm_names)/sizeof(bm_names[0]);

	for (i=0; i<len; i++) {
		if (strstr(name, bm_names[i]))
			return 1;
	}
	return 0;
}



void spcd_page_fault_handler(struct kprobe *kp, struct pt_regs *regs, unsigned long flags)
{
	struct task_struct *task = current;
	unsigned long address = read_cr2();
	int tid = pt_get_tid(task->pid);

	if (tid > -1){
		spin_lock(&ptl2);
		pt_pf++;
		pt_check_comm(tid, address);
		spin_unlock(&ptl2);
	}	

}


int spcd_pte_fault_handler(struct task_struct *task, struct mm_struct *mm,
		     struct vm_area_struct *vma, unsigned long address,
		     pte_t *pte, pmd_t *pmd, unsigned int flags)
{

	struct pt_mem_info *elem;

	if (!pt_thread || !pt_task || pt_task->mm != mm)
		jprobe_return();

	spin_lock(&ptl);

	elem = pt_get_mem(address);
	if (elem->pte_cleared)
		pt_fix_pte(elem, address);

	spin_unlock(&ptl);

	jprobe_return();
	return 0; /* not reached */
}


int spcd_exit_process_handler(struct task_struct *task)
{
	int tid;

	if (pt_task == task) {
		pt_reset();
		printk("pt: stop %s (pid %d)\n", task->comm, task->pid);
		pt_print_stats();
		pt_reset_stats();
		jprobe_return();
	}

	tid = pt_get_tid(task->pid);
	if (tid >-1){
		pt_delete_pid(task->pid, tid);
	}

	jprobe_return();
	return 0;
}


int spcd_new_process_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int ret = regs_return_value(regs);
	struct task_struct *task = current;
	
	if (pt_task == 0 && ret == 0) {
		if (spcd_check_name(task->comm)) {
			printk("\npt: start %s (pid %d)\n", task->comm, task->pid);
			pt_add_pid(task->pid, pt_num_threads);
			pt_task = task;
		}
	}

	return 0;
}


int spcd_fork_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int pid = regs_return_value(regs);
	struct pid *pids = find_get_pid(pid);
	struct task_struct *task = pid_task(pids, PIDTYPE_PID);

	if (pt_task && task && pt_task->parent->pid == task->parent->pid) {
		pt_add_pid(pid, pt_num_threads);
		if (!pt_thread) {
			// pt_thread = kthread_create(pt_pf_thread_func, NULL, "pt_pf_thread");
			// wake_up_process(pt_thread);
		}
	}

	return 0;
}


static struct jprobe spcd_pte_fault_jprobe = {
	.entry = spcd_pte_fault_handler,
	.kp.symbol_name = "handle_pte_fault",
};


static struct kprobe spcd_page_fault_probe = {
	.post_handler = spcd_page_fault_handler
};


static struct jprobe spcd_exit_process_probe = {
	.entry = spcd_exit_process_handler,
	.kp.symbol_name = "exit_mm",
};

static struct kretprobe spcd_new_process_probe = {
	.handler = spcd_new_process_handler,
	.kp.symbol_name = "do_execve",
};

static struct kretprobe spcd_fork_probe = {
	.handler = spcd_fork_handler,
	.kp.symbol_name = "do_fork",
};


int init_module(void)
{
	printk("Welcome.....\n");
	pt_reset_stats();

	// check if we can use jprobes/kretprobes here:
	spcd_page_fault_probe.addr = (kprobe_opcode_t *) kallsyms_lookup_name("do_page_fault") + 0x5d;


	register_jprobe(&spcd_pte_fault_jprobe);
	register_kprobe(&spcd_page_fault_probe);
	register_jprobe(&spcd_exit_process_probe);
	register_kretprobe(&spcd_new_process_probe);
	register_kretprobe(&spcd_fork_probe);

	return 0;
}


void cleanup_module(void)
{
	unregister_jprobe(&spcd_pte_fault_jprobe);
	unregister_kprobe(&spcd_page_fault_probe);
	unregister_jprobe(&spcd_exit_process_probe);
	unregister_kretprobe(&spcd_new_process_probe);
	unregister_kretprobe(&spcd_fork_probe);

	printk("Bye.....\n");
}


void pt_reset(void)
{
	if (pt_thread)
		kthread_stop(pt_thread);
	pt_task = 0;
	pt_thread = NULL;
}


void pt_reset_stats(void)
{
	pt_pid_clear();
	pt_mem_clear();
	pt_pte_fixes = 0;
	pt_num_threads = 0;
	pt_num_walks = 0;
	pt_pf = 0;
	pt_pf_extra = 0;
	pt_addr_conflict = 0;
	pt_next_addr = 0;
	pt_next_vma = NULL;
	memset(share, 0, sizeof(share));
}


int pt_pf_thread_func(void* v)
{
	
	while (1) {
		if (kthread_should_stop())
			return 0;
		pt_pf_pagewalk(pt_task->mm);
		msleep(10);
	}
	
}


void pt_print_stats(void)
{
	int i,j;

	printk("(%d threads): %lu pfs (%lu extra, %lu fixes), %lu walks, %lu addr conflicts\n", pt_num_threads, pt_pf, pt_pf_extra, pt_pte_fixes, pt_num_walks, pt_addr_conflict);

	for (i = pt_num_threads-1; i >= 0; i--) {
		for (j = 0; j < pt_num_threads; j++){
			printk ("%lu", share[i][j] + share[j][i]);
			if (j != pt_num_threads-1)
				printk (",");
		}
		printk("\n");
	}
}
