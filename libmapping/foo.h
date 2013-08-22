#ifndef __LIBMAPPING_FOO_H__
#define __LIBMAPPING_FOO_H__

#ifndef MAX_THREADS
	#error MAX_THREADS should be defined
#endif

#ifdef LIBMAPPING_DEBUG
	#define DEBUG
#endif

#define HAVE_TLS

#ifndef LIBMAPPING_MODE_PINSAMMU
	#define PRINTF_PREFIX "libmapping: "
#else
	#define PRINTF_PREFIX "libmapping-pin-sammu: "
#endif

#define PRINTF_UINT64  "%" PRIu64

#ifdef _SPCD
	typedef unsigned long long uint64_t;
#endif

#endif
