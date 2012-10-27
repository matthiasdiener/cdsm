#include "pt_comm.h"

MODULE_LICENSE("GPL");

unsigned long pt_pf;
unsigned long pt_addr_conflict;
unsigned long pt_pf_extra;
unsigned pt_num_faults;
atomic_t pt_num_threads = ATOMIC_INIT(0);
atomic_t pt_active_threads = ATOMIC_INIT(0);
unsigned long pt_num_walks;

struct task_struct *pt_task;

unsigned pt_num_faults = 3;
unsigned long pt_pte_fixes;

unsigned long pt_next_addr;
struct vm_area_struct *pt_next_vma;

unsigned long share [PT_MAXTHREADS][PT_MAXTHREADS];

struct task_struct *pt_thread;

DEFINE_SPINLOCK(ptl);

DEFINE_SPINLOCK(ptl_check_comm);

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

void pt_maybe_fix_pte(unsigned long address)
{
	struct pt_mem_info *elem;

	spin_lock(&ptl);

	elem = pt_get_mem(address);
	if (elem->pte_cleared)
		pt_fix_pte(elem, address);

	spin_unlock(&ptl);
}


int spcd_pte_fault_handler(struct task_struct *task, struct mm_struct *mm,
		     struct vm_area_struct *vma, unsigned long address,
		     pte_t *pte, pmd_t *pmd, unsigned int flags)
{
	int tid;

	if (!pt_task || pt_task->mm != mm)
		jprobe_return();

	pt_maybe_fix_pte(address);

	tid = pt_get_tid(task->pid);
	if (tid > -1){
		spin_lock(&ptl_check_comm);
		pt_pf++;
		pt_check_comm(tid, address);
		spin_unlock(&ptl_check_comm);
	}

	jprobe_return();
	return 0; /* not reached */
}


int spcd_exit_process_handler(long code)
{
	struct task_struct *task = current;
	int tid = pt_get_tid(task->pid);

	if (tid > -1) {
		pt_delete_pid(task->pid, tid);
		if (atomic_read(&pt_active_threads) == 0) {
			pt_reset();
			printk("pt: stop %s (pid %d)\n", task->comm, task->pid);
			pt_print_stats();
			pt_reset_stats();
		}
	}

	jprobe_return();
	return 0; /* not reached */
}


int spcd_new_process_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int ret = regs_return_value(regs);
	struct task_struct *task = current;

	if (pt_task || ret)
		return 0;


	if (spcd_check_name(task->comm)) {
		printk("\npt: start %s (pid %d)\n", task->comm, task->pid);
		pt_add_pid(task->pid, atomic_read(&pt_num_threads));
		pt_task = task;
	}


	return 0;
}


int spcd_fork_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int pid = regs_return_value(regs);
	struct pid *pids;
	struct task_struct *task = NULL;

	rcu_read_lock();
	pids = find_vpid(pid);
	if (pids)
		task = pid_task(pids, PIDTYPE_PID);
	rcu_read_unlock();

	if (!task)
		return 0;

	if (pt_task && pt_task->parent->pid == task->parent->pid) {
		pt_add_pid(pid, atomic_read(&pt_num_threads));
	}

	return 0;
}


static struct jprobe spcd_pte_fault_jprobe = {
	.entry = spcd_pte_fault_handler,
	.kp.symbol_name = "handle_pte_fault",
};

static struct jprobe spcd_exit_process_probe = {
	.entry = spcd_exit_process_handler,
	.kp.symbol_name = "do_exit",
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

	register_jprobe(&spcd_pte_fault_jprobe);
	register_jprobe(&spcd_exit_process_probe);
	register_kretprobe(&spcd_new_process_probe);
	register_kretprobe(&spcd_fork_probe);

	pt_thread = kthread_create(pt_pf_thread_func, NULL, "pt_pf_thread");
	wake_up_process(pt_thread);

	return 0;
}


void cleanup_module(void)
{
	if (pt_thread)
		kthread_stop(pt_thread);

	unregister_jprobe(&spcd_pte_fault_jprobe);
	unregister_jprobe(&spcd_exit_process_probe);
	unregister_kretprobe(&spcd_new_process_probe);
	unregister_kretprobe(&spcd_fork_probe);

	printk("Bye.....\n");
}


void pt_reset(void)
{
	pt_task = NULL;
	//pt_thread = NULL;
}


void pt_reset_stats(void)
{
	pt_pid_clear();
	pt_mem_clear();
	pt_pte_fixes = 0;
	atomic_set(&pt_num_threads, 0);
	atomic_set(&pt_active_threads, 0);
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
		if (pt_task)
			pt_pf_pagewalk(pt_task->mm);
		msleep(10);
	}
	
}


void pt_print_stats(void)
{
	int i, j;
	int nt = atomic_read(&pt_num_threads);

	printk("(%d threads): %lu pfs (%lu extra, %lu fixes), %lu walks, %lu addr conflicts\n", nt, pt_pf, pt_pf_extra, pt_pte_fixes, pt_num_walks, pt_addr_conflict);
	return;
	for (i = nt-1; i >= 0; i--) {
		for (j = 0; j < nt; j++){
			printk ("%lu", share[i][j] + share[j][i]);
			if (j != nt-1)
				printk (",");
		}
		printk("\n");
	}
}
