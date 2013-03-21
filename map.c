#include "spcd.h"

extern void map_simple(void);
extern void map_drake(int nt);

int spcd_map_func(void* v)
{
	int nt;
	
	while (1) {
		if (kthread_should_stop())
			return 0;
		
		nt = spcd_get_active_threads();
		if (nt > 2) {
			// pt_print_share();
			map_drake(nt);
			//pt_share_clear();
		}
		msleep(500);
	}
	return 0;
}
