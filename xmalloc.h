// xmalloc.h: Succeed-or-die allocation routines to replace libexplain
#pragma once
#include <stdlib.h>
#include <stdio.h>

#ifndef __attribute_malloc__
#define __attribute_malloc__
#endif

#define X(f, argsDecl, args, doCheck) __attribute_malloc__ void *x##f argsDecl \
	{ void *x = f args; if((doCheck) && !x) { perror(#f); exit(EXIT_FAILURE); } return x; }

X(malloc, (size_t siz), (siz), siz)
X(calloc, (size_t num, size_t siz), (num, siz), num && siz)
X(realloc, (void *ptr, size_t siz), (ptr, siz), siz)
