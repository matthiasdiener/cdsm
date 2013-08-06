#include "spcd.h"
#include "libspcd.h"

#include <linux/seq_file.h>

static struct proc_dir_entry *spcd_proc_root;

static
ssize_t matrix_reset(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	spin_lock(&spcd_main_matrix.lock);
	if (!spcd_main_matrix.matrix){
		spcd_main_matrix.matrix = (unsigned*) kmalloc (sizeof(unsigned) * max_threads * max_threads, GFP_KERNEL);
	}

	memset(spcd_main_matrix.matrix, 0, sizeof(unsigned) * max_threads * max_threads);
	spin_unlock(&spcd_main_matrix.lock);

	return count;
}

static
int matrix_read(struct seq_file *m, void *v)
{
	int i, j;
	int nt = spcd_get_num_threads();

	if (nt < 2)
		return 1;

	/* TODO: put this into separate proc file
	for (i = nt-1; i >= 0; i--) {
		seq_printf(m, "%u", pt_get_pid(i));
		if (i != 0)
			seq_printf(m, ",");
	}
	seq_printf(m, "\n\n");
	*/

	for (i = nt-1; i >= 0; i--) {
		for (j = 0; j < nt; j++) {
			seq_printf(m, "%u", get_share(i, j));
			if (j != nt-1)
				seq_printf(m, ",");
		}
		seq_printf(m, "\n");
	}

	return 0;
}


/* TODO: fix this for new procfs API
static
int matrix_read_raw(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	static void *tmp_buffer = NULL;
	static size_t tmp_buffer_size;
	spcd_matrix_t temp;
	void *ptr;
	int i;

	if(offset == 0){
		if(tmp_buffer)
			kfree(tmp_buffer);

		temp.num_threads = spcd_get_num_threads();
		temp.max_threads = max_threads;
		temp.pids = kmalloc(sizeof(int) * temp.num_threads, GFP_KERNEL);

		for (i = temp.num_threads-1; i >= 0; i--) {
			temp.pids[i] = pt_get_pid(temp.num_threads - 1 - i);
		}

		spin_lock(&spcd_main_matrix.lock);
		temp.matrix = spcd_main_matrix.matrix;
		ptr = spcd_matrix_encode(&temp);
		tmp_buffer_size = spcd_matrix_size(ptr);
		tmp_buffer = kmalloc(tmp_buffer_size, GFP_KERNEL);
		memcpy(tmp_buffer, ptr, tmp_buffer_size);
		spin_unlock(&spcd_main_matrix.lock);

		kfree(ptr);
		kfree(temp.pids);
	}

	if (count < (tmp_buffer_size - offset)){
		memcpy(buf, tmp_buffer+offset, count);
		return count;
	} else {
		memcpy(buf, tmp_buffer+offset, tmp_buffer_size - offset);
		*eof = 1;
		return tmp_buffer_size - offset;
	}
}
*/

static
int matrix_open(struct inode *inode, struct file *file)
{
	return single_open(file, matrix_read, NULL);
}

static const struct file_operations matrix_ops = {
	.owner = THIS_MODULE,
	.open = matrix_open,
	.read = seq_read,
	.llseek	= seq_lseek,
	.release = single_release,
};

static const struct file_operations reset_ops = {
	.owner = THIS_MODULE,
	.write = matrix_reset,
};

int spcd_proc_init(void)
{
	spcd_proc_root = proc_mkdir("spcd", NULL);
	if (!spcd_proc_root) {
		printk(KERN_INFO "SPCD BUG: error creating proc root entry");
		return -ENOMEM;
	}

	proc_create("matrix", 0, spcd_proc_root, &matrix_ops);
	// proc_create("raw_matrix", 0, spcd_proc_root, spcd_read_raw_matrix, NULL);
	proc_create("reset", 0666, spcd_proc_root, &reset_ops);

	return 0;
}


void spcd_proc_cleanup(void)
{
	remove_proc_entry("reset", spcd_proc_root);
	// remove_proc_entry("raw_matrix", spcd_proc_root);
	remove_proc_entry("matrix", spcd_proc_root);
	remove_proc_entry("spcd",NULL);
}
