#pragma once
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include "xmalloc.h"

#define CLONE(func, type) type *func(type x) { type *y = xmalloc(sizeof(x)); *y = x; return y; }
#define FREE_P(xfree, type) void xfree##P (type *x) { if(x) { xfree(*x); free(x); } }

/* like printf, but prints onto stderr and exits with EXIT_FAILURE. */
__attribute__((noreturn))
__attribute__((format(printf, 1, 2)))
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

/** Prints an array of ints and (if needed) their sum. Prints a newline afterwards.
	@returns The sum
*/
int prSum(const signed int *ls, size_t c)
{
	prlsd(ls, c, " + ");
	int n = sumls(ls, c);

	if(c > 1)
		printf(" = %d", n);

	putchar('\n');
	return n;
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

// the integral 𝜙(x) of a normal distribution with the given 𝜇 and 𝜎
double phi(double x, double mu, double sigma)
{
	return 0.5 * erfc((mu - x) / (sqrt(2) * sigma));
}

// The probability of x being rolled on a rounded normal distribution with the given 𝜇 and 𝜎
double normal(double mu, double sigma, int x)
{
	return phi(x + 0.5, mu, sigma) - phi(x - 0.5, mu, sigma);
}
