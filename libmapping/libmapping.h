#ifndef __LIBMAPPING_H__
#define __LIBMAPPING_H__

#ifndef _SPCD
	#ifndef _GNU_SOURCE
		#define _GNU_SOURCE
	#endif
	#include <stdio.h>
	#include <inttypes.h>
	#include <stdint.h>
	#include <stdlib.h>
	#include <string.h>
	#include <limits.h>
	#include <sched.h>
	#include <errno.h>
	#include <sys/time.h>
	#include <unistd.h>
	#include <wctype.h>
	#include <zlib.h>
#ifdef LIBMAPPING_DATA_MAPPING
	#include <sys/mman.h>
	#include <numaif.h>
#endif
	#include <signal.h>
#else
	#include <linux/slab.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include "foo.h"
#include "lib.h"
#include "graph.h"
#include "mapping.h"
#include "topology.h"
#include "connect.h"
#include "mapping-algorithms.h"

#ifndef _SPCD
	#include "env.h"
	#include "statistics.h"
	#include "mapping-filters.h"
#ifdef LIBMAPPING_DATA_MAPPING
	#include "elf.h"
	#include "datamap.h"
#endif
	#if defined(LIBMAPPING_MODE_CONSUME) || defined(LIBMAPPING_MODE_PRODUCE)
		#include "io.h"
		#include "lex.h"
		#include "produce-consume-mode.h"
	#endif
#endif

#ifdef __cplusplus
}
#endif

#endif
