#include "pt_comm.h"

dotraplinkage void 
spcd_page_fault(struct pt_regs *regs, unsigned long error_code)
{
	// spinlock_t *ptl;
	// struct vm_area_struct *vma;
	struct task_struct *tsk;
	unsigned long address;
	struct mm_struct *mm;
	// int fault;
	// int write = error_code & PF_WRITE;
	// unsigned int flags = FAULT_FLAG_ALLOW_RETRY | FAULT_FLAG_KILLABLE |
					// (write ? FAULT_FLAG_WRITE : 0);
	// int pt_do, tid = -1;


	tsk = current;
	mm = tsk->mm;

	/* Get the faulting address: */
	address = read_cr2();
	__sync_add_and_fetch(&pt_pf, 1);

	if ((pt_pf % 100) == 0)
		printk("mpf\n");
	do_page_fault_original(regs, error_code);

}


void pt_check_comm(struct task_struct *tsk, unsigned long address)
{
	DEFINE_SPINLOCK(ptl);
	int mytid;
	struct pt_mem_info *elem;

	// detection not requested -> return
	if (pt_task == 0)
		return;
	
	mytid = pt_get_tid(tsk->pid);
	// thread not in list -> we are not interested
	if (mytid == -1)
		return;


	elem = pt_get_mem(address);

	spin_lock(&ptl);

	if (elem->pg_addr != (address >> PAGE_SHIFT) ){
		if (elem->pg_addr !=0 ) {
			// printk ("XXX conflict, hash = %ld, old = %lu, new = %lu\n", h, elem->pg_addr, (address >> PAGE_SHIFT));
			pt_addr_conflict++;
		}
		elem->sharer[0] = -1;
		elem->sharer[1] = -1;
		elem->pg_addr = address >> PAGE_SHIFT;
	}

	// no sharer present
	if (elem->sharer[0] == -1 && elem->sharer[1] == -1) {
		elem->sharer[0] = mytid;
		goto out;
	}

	// two sharers present
	if (elem->sharer[0] != -1 && elem->sharer[1] != -1) {

		// both different from myself
		if (elem->sharer[0] != mytid && elem->sharer[1] != mytid) {
			share[mytid][elem->sharer[0]] ++;
			share[mytid][elem->sharer[1]] ++;
			
			elem->sharer[1] = elem->sharer[0];
			elem->sharer[0] = mytid;

			goto out;
		}


		if (elem->sharer[0] == mytid) {
			share[mytid][elem->sharer[1]] ++;
			goto out;
		}

		if (elem->sharer[1] == mytid) {
			share[mytid][elem->sharer[0]] ++;
			goto out;
		}


		goto out;
	}

	// only second sharer present
	if (elem->sharer[0] == -1) {
		if (elem->sharer[1] != mytid) {
			elem->sharer[0] = mytid;
			share[mytid][elem->sharer[1]] ++;
		}
		goto out;
	}

	// only first sharer present
	if (elem->sharer[0] != mytid) {
		elem->sharer[1] = mytid;
		share[mytid][elem->sharer[0]] ++;
	}

	out:
		spin_unlock(&ptl);

}


void pt_print_comm(void)
{
	int i,j;
	int nt = pt_get_numthreads();
	for (i = nt-1; i >= 0; i--) {
		for (j = 0; j < nt; j++){
			printk ("%lu", share[i][j]+share[j][i]);
			if (j != nt-1)
				printk (",");
		}
		printk("\n");
	}
}