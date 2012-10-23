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

static void (*spcd_new_process_original_ref)(struct task_struct *); 
extern void (*spcd_new_process)(struct task_struct *);

static void (*spcd_exit_process_original_ref)(struct task_struct *); 
extern void (*spcd_exit_process)(struct task_struct *);

static DEFINE_SPINLOCK(ptl);

int pt_check_name(char *name)
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



void pt_dpf_handler(struct kprobe *kp, struct pt_regs *regs, unsigned long flags)
{
	
}

static struct kprobe pt_dpf_probe = {
	.post_handler = pt_dpf_handler
};

int pt_pte_fault(struct task_struct *task, struct mm_struct *mm,
		     struct vm_area_struct *vma, unsigned long address,
		     pte_t *pte, pmd_t *pmd, unsigned int flags)
{

	struct pt_mem_info *elem;
	int tid;

	if (!pt_task || pt_task->mm != mm)
		goto out;

	elem = pt_get_mem(address);
	if (elem->pte_cleared) {
		pt_fix_pte(address);
		elem->pte_cleared = 0;
	}

	tid = pt_get_tid(task->pid); 
	if (tid > -1) {
		spin_lock(&ptl);
		pt_pf++;
		elem = pt_check_comm(tid, address);
		spin_unlock(&ptl);
	}

	out:
	jprobe_return();
	return 0; /* not reached */
}

struct jprobe pt_jprobe = {
	.entry = (kprobe_opcode_t *) pt_pte_fault
};

void spcd_exit_process_new(struct task_struct *task)
{
	int tid;

	if (pt_task == task) {
		pt_reset();
		printk("pt: stop %s (pid %d)\n", task->comm, task->pid);
		pt_print_stats();
		pt_reset_stats();
		return;
	}
	tid = pt_get_tid(task->pid);
	if (tid >-1){
		pt_delete_pid(task->pid, tid);
	}
}


void spcd_new_process_new(struct task_struct *task)
{
	if (pt_task == 0) {
		if (pt_check_name(task->comm)) {
			printk("pt: start %s (pid %d)\n", task->comm, task->pid);
			pt_add_pid(task->pid, pt_num_threads);
			pt_task = task;
		}
	} else if (pt_task->parent->pid == task->parent->pid) {
		pt_add_pid(task->pid, pt_num_threads);
		if (!pt_thread) {
			pt_thread = kthread_create(pt_pf_thread_func, NULL, "pt_pf_thread");
			wake_up_process(pt_thread);
		}
	}
}


int init_module(void)
{
	printk("Welcome.....\n");
	pt_reset_stats();

	spcd_new_process_original_ref = spcd_new_process;
	spcd_new_process = &spcd_new_process_new;

	spcd_exit_process_original_ref = spcd_exit_process;
	spcd_exit_process = &spcd_exit_process_new;

	pt_jprobe.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("handle_pte_fault"); //not necessary
	pt_dpf_probe.addr = (kprobe_opcode_t *) kallsyms_lookup_name("do_page_fault") + 0x5D;

	register_kprobe(&pt_dpf_probe);
	register_jprobe(&pt_jprobe);

	return 0;
}


void cleanup_module(void)
{
	spcd_new_process = spcd_new_process_original_ref;
	spcd_exit_process = spcd_exit_process_original_ref;
	unregister_jprobe(&pt_jprobe);
	printk("Bye.....\n");
}


void pt_reset(void)
{
	kthread_stop(pt_thread);
	pt_task = 0;
	pt_thread = 0;
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
