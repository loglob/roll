#include <stdio.h>
#include <stdlib.h>

#ifndef __attribute_malloc__
#define __attribute_malloc__
#endif

#ifndef X
#define X(f, argsDecl, args, doCheck) __attribute_malloc__ void *x##f argsDecl \
	{ void *x = f args; if((doCheck) && !x) { perror(#f); exit(EXIT_FAILURE); } return x; }
#endif

X(malloc, (size_t siz), (siz), siz)
X(calloc, (size_t num, size_t siz), (num, siz), num && siz)
X(realloc, (void *ptr, size_t siz), (ptr, siz), siz)

#undef X
