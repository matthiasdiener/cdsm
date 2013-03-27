#include "spcd.h"
#include "libspcd.h"

static struct proc_dir_entry *spcd_proc_root;


int spcd_proc_write_reset(struct file *file, const char *buf, unsigned long count, void *data)
{
	spin_lock(&spcd_main_matrix.lock);
	if (!spcd_main_matrix.matrix){
		spcd_main_matrix.matrix = (unsigned*) kmalloc (sizeof(unsigned) * max_threads * max_threads, GFP_KERNEL);
	}
	
	memset(spcd_main_matrix.matrix, 0, sizeof(unsigned) * max_threads * max_threads);
	spin_unlock(&spcd_main_matrix.lock);
	
	return count;
}


int spcd_read_matrix(char *buf, char **start, off_t offset, int count, int *eof, void *data) 
{
	int i, j;
	int s;
	int nt = spcd_get_num_threads();
	int len=0;

	if (nt < 2)
		return 0;

	for (i = nt-1; i >= 0; i--) {
		len += sprintf(buf+len, "%u", pt_get_pid(i));
		if (i != 0)
			len += sprintf(buf+len, ",");
	}
	len += sprintf(buf+len, "\n\n");

	for (i = nt-1; i >= 0; i--) {
		for (j = 0; j < nt; j++) {
			spin_lock(&spcd_main_matrix.lock);
			s = i > j ? spcd_main_matrix.matrix[i*max_threads + j] : spcd_main_matrix.matrix[j*max_threads + i];
			spin_unlock(&spcd_main_matrix.lock);
			len += sprintf(buf+len, "%u", s);
			if (j != nt-1)
				len += sprintf(buf+len, ",");
		}
		len += sprintf(buf+len, "\n");
	}

	return len;
}


int spcd_read_raw_matrix(char *buf, char **start, off_t offset, int count, int *eof, void *data) 
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

	if(count < (tmp_buffer_size - offset)){
		memcpy(buf,tmp_buffer+offset,count);
		return count;
	}
	else{
		memcpy(buf,tmp_buffer+offset,tmp_buffer_size - offset);
		*eof = 1;
		return tmp_buffer_size - offset;
	}
}


int spcd_proc_init(void)
{	
	spcd_proc_root = proc_mkdir("spcd", NULL);
	if(!spcd_proc_root)
	{
		printk(KERN_INFO "Error creating proc root entry");
		return -ENOMEM;
	}
	
	create_proc_read_entry("raw_matrix", 0,spcd_proc_root,spcd_read_raw_matrix, NULL);
	create_proc_read_entry("matrix", 0,spcd_proc_root,spcd_read_matrix, NULL);
	create_proc_entry("reset", 0666, spcd_proc_root)->write_proc = spcd_proc_write_reset;

	return 0;
}


void spcd_proc_cleanup(void)
{
	remove_proc_entry("reset", spcd_proc_root);
	remove_proc_entry("raw_matrix", spcd_proc_root);
	remove_proc_entry("matrix", spcd_proc_root);
	remove_proc_entry("spcd",NULL);
}
