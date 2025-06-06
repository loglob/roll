#include "util.h"
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


NORETURN_ATTR
PRINTF_ATTR(1,2)
void eprintf(const char *fmt, ...)
{
	va_list v;
	va_start(v, fmt);
	vfprintf(stderr, fmt, v);
	exit(EXIT_FAILURE);
	__builtin_unreachable();
}

void prlsd(const signed int *ls, size_t c, const char *del)
{
	for (size_t i = 0; i < c; i++)
		i ? printf("%s%d", del, ls[i]) : printf("%d", ls[i]);
}

void prls(const int *ls, size_t c)
{
	prlsd(ls, c, ", ");
}

int sumls(const signed int *ls, size_t c)
{
	signed int sum = 0;

	for (size_t i = 0; i < c; i++)
		sum += ls[i];

	return sum;
}

int prSum(const signed int *ls, size_t c)
{
	prlsd(ls, c, " + ");
	int n = sumls(ls, c);

	if(c > 1)
		printf(" = %d", n);

	putchar('\n');
	return n;
}

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

int intpcomp(const void *l, const void *r)
{
	return *(const int*)l - *(const int*)r;
}

signed int max(signed int a, signed int b)
{
	return (a > b) ? a : b;
}

signed int min(signed int a, signed int b)
{
	return (a > b) ? b : a;
}

signed int min0(signed int a, signed int b)
{
	return a && (!b || a < b) ? a : b;
}

double phi(double x, double mu, double sigma)
{
	return 0.5 * erfc((mu - x) / (sqrt(2) * sigma));
}

double normal(double mu, double sigma, int x)
{
	return phi(x + 0.5, mu, sigma) - phi(x - 0.5, mu, sigma);
}

#define X(f, argsDecl, args, doCheck) MALLOC_ATTR void *x##f argsDecl \
	{ void *x = f args; if((doCheck) && !x) { perror(#f); exit(EXIT_FAILURE); } return x; }

X(malloc, (size_t siz), (siz), siz)
X(calloc, (size_t num, size_t siz), (num, siz), num && siz)
X(realloc, (void *ptr, size_t siz), (ptr, siz), siz)

void *recalloc(void *ptr, size_t num, size_t siz)
{
	if(siz > 0 && num > SIZE_MAX / siz)
		eprintf("perror: Overflow in size calculation");
	
	return realloc(ptr, num * siz);
}

#undef X
