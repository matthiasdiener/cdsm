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
