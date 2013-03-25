#include "spcd.h"
#include "spcd_proc.h"
#include "libspcd.h"

static struct proc_dir_entry *spcd_proc_root;
static struct proc_dir_entry *spcd_matrix;
static struct proc_dir_entry *spcd_raw_matrix;
static struct proc_dir_entry *spcd_pnames;

static DEFINE_SPINLOCK(spcd_procfs_lock);

struct spcd_names_list spcd_current_pnames;

static void *tmp_buffer;
size_t tmp_buffer_size;

int spcd_procfs_get_names(char ***pnames){
	//[TODO] check possible race condition
	//[TODO] possible bottleneck
	
	char **names;
	struct spcd_names_list *tmp;
	int counter = 0;
	
	spin_lock(&spcd_procfs_lock);
	
	list_for_each_entry(tmp, &spcd_current_pnames.list, list){
		counter++;
	}
	
	names = kmalloc(counter * sizeof(char *), GFP_KERNEL);
	printk("%p %p", names, pnames);
	counter = 0;
	list_for_each_entry(tmp, &spcd_current_pnames.list, list){
		names[counter] = kmalloc(sizeof(tmp->name), GFP_KERNEL);
		memcpy(names[counter], tmp->name, sizeof(tmp->name));
		counter++;
	}
	
	*pnames = names;
	
	spin_unlock(&spcd_procfs_lock);
	
	return counter;
}

void spcd_procfs_add_name(const char *name){
	struct spcd_names_list *tmp;
	
	printk(KERN_INFO "SPCD: Adding '%s' to list of monitored processes\n", name);
	
	spin_lock(&spcd_procfs_lock);
	
	tmp = (struct spcd_names_list *) kmalloc(sizeof(struct spcd_names_list), GFP_KERNEL);
	tmp->name = (char *) kmalloc(sizeof(name), GFP_KERNEL);
	memcpy(tmp->name, name, sizeof(name));
	list_add(&(tmp->list), &(spcd_current_pnames.list));
	
	spin_unlock(&spcd_procfs_lock);
}

int spcd_proc_read_pnames(char *buf,char **start,off_t offset,int count,int *eof,void *data )
{
	int i, len=0;
	char **pnames;
	int num = spcd_procfs_get_names(&pnames);

	for(i=0;i<num;i++)
 		len += sprintf(buf+len,"%s\n",pnames[i]);

	return len;
}

int spcd_proc_write_pnames(struct file *file,const char *buf, unsigned long count,void *data )
{
	int i=0;
	char *my_buf = kmalloc(count * sizeof(char), GFP_KERNEL);
	
	for(;i < count && *buf && *buf != '\n' && *buf != '\r' && count > 0;i++)
		my_buf[i] = *(buf++);
	my_buf[i] = 0;
	
	spcd_procfs_add_name(my_buf);
	
	return count;
}

int spcd_proc_create_pnames_entry(void)
{
	spcd_pnames = create_proc_entry("monitored_processes",0666,spcd_proc_root);
	if(!spcd_pnames)
	{
	    printk(KERN_INFO "Error creating proc entry");
	    return -ENOMEM;
	}
	spcd_pnames->read_proc = spcd_proc_read_pnames;
	spcd_pnames->write_proc = spcd_proc_write_pnames;

	return 0;
}

int spcd_read_matrix(char *buf,char **start,off_t offset,int count,int *eof,void *data ) 
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

int spcd_read_raw_matrix(char *buf,char **start,off_t offset,int count,int *eof,void *data ) 
{
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
			temp.pids[i] = pt_get_pid(temp.num_threads - i);
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

int spcd_proc_create_matrix_entry(void)
{
	spcd_matrix = create_proc_read_entry("matrix",0,spcd_proc_root,spcd_read_matrix,NULL);
	
	return 0;
}

int spcd_proc_create_raw_matrix_entry(void)
{
	spcd_raw_matrix = create_proc_read_entry("raw_matrix",0,spcd_proc_root,spcd_read_raw_matrix,NULL);
	
	return 0;
}

int spcd_proc_init (void) 
{
	tmp_buffer = NULL;
	
	spcd_proc_root = proc_mkdir("spcd",NULL);
	if(!spcd_proc_root)
	{
	    printk(KERN_INFO "Error creating proc root entry");
	    return -ENOMEM;
	}		
	
	INIT_LIST_HEAD(&spcd_current_pnames.list);
	
	spcd_proc_create_raw_matrix_entry();
	spcd_proc_create_matrix_entry();
	spcd_proc_create_pnames_entry();
	
    return 0;
}

void spcd_proc_cleanup(void) 
{
    remove_proc_entry("monitored_processes", spcd_proc_root);
	remove_proc_entry("raw_matrix", spcd_proc_root);
	remove_proc_entry("matrix", spcd_proc_root);
	remove_proc_entry("spcd",NULL);
}