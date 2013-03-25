#include <linux/proc_fs.h>
#include <linux/slab.h>

#ifndef SPCD_PROC_H
#define SPCD_PROC_H

struct spcd_names_list{
	char* name;
	struct list_head list;
};

int spcd_proc_init (void);
void spcd_proc_cleanup(void);
void spcd_update_matrix(const char* matrix, int len);

int spcd_procfs_get_names(char **names[]);

#endif /* end of include guard: SPCD_PROC_H */