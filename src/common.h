#ifndef _COMMON_H_
#define _COMMON_H_
#include <linux/limits.h>
#include <stdio.h>

#define FAIL(text) do {			\
	fprintf(stderr, text "\n");	\
	return 1;					\
} while(0)


#endif // _COMMON_H_