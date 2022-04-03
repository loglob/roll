#pragma once
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "xmalloc.h"

/* Determines if element is in the array set of length length */
bool in(int length, int set[length], int element)
{
	for (int i = 0; i < length; i++)
	{
		if(element == set[i])
			return true;
	}

	return false;
}

/* like printf, but prints onto stderr and exits with EXIT_FAILURE. */
__attribute__((noreturn))
void eprintf(const char *fmt, ...)
{
	va_list v;
	va_start(v, fmt);
	vfprintf(stderr, fmt, v);
	exit(EXIT_FAILURE);
	__builtin_unreachable();
}

/* prints an array of ints with a custom delimiter */
void prlsd(const signed int *ls, size_t c, const char *del)
{
	for (size_t i = 0; i < c; i++)
		i ? printf("%s%d", del, ls[i]) : printf("%d", ls[i]);
}

/* prints an array of ints, separated by ", " */
void prls(const int *ls, size_t c)
{
	prlsd(ls, c, ", ");
}

/* sums an array of ints */
int sumls(const signed int *ls, size_t c)
{
	signed int sum = 0;

	for (size_t i = 0; i < c; i++)
		sum += ls[i];

	return sum;
}

/* like strol, but with int instead of long */
signed int strtoi(const char *inp, char **end, int base)
{
	errno = 0;
	signed long v = strtol(inp, (char**)end, base);

	if(errno)
		return 0;
	if(v > INT_MAX)
	{
		errno = ERANGE;
		return INT_MAX;
	}
	if(v < INT_MIN)
	{
		errno = ERANGE;
		return INT_MIN;
	}
	return (int)v;
}

/* like stroul, but with int instead of long */
signed int strtoui(const char *inp, char **end, int base)
{
	errno = 0;
	unsigned long v = strtoul(inp, (char**)end, base);

	if(errno)
		return 0;
	if(v > UINT_MAX)
	{
		errno = ERANGE;
		return UINT_MAX;
	}
	return (int)v;
}

/* Compares the dereferenced values of two int pointers, for qsort() */
int intpcomp(const void *l, const void *r)
{
	return *(const int*)l - *(const int*)r;
}

/* The maximum of a and b */
signed int max(signed int a, signed int b)
{
	return (a > b) ? a : b;
}

/* The minimum of a and b */
signed int min(signed int a, signed int b)
{
	return (a > b) ? b : a;
}

// like min(), but evaluates 0 as larger than every other value.
signed int min0(signed int a, signed int b)
{
	return a && (!b || a < b) ? a : b;
}
