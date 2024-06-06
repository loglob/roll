#pragma once
#include <stddef.h>


#define CLONE_SIG(func, type) __attribute__((malloc, returns_nonnull, leaf)) type *func(type x);
#define CLONE(func, type) type *func(type x) { type *y = xmalloc(sizeof(x)); *y = x; return y; }
#define FREE_SIG(xfree, type) void xfree##P;
#define FREE_P(xfree, type) void xfree##P (type *x) { if(x) { xfree(*x); free(x); } }

/** like printf, but prints onto stderr and exits with EXIT_FAILURE. */
__attribute__((noreturn))
__attribute__((format(printf, 1, 2)))
void eprintf(const char *fmt, ...);

/** prints an array of ints with a custom delimiter */
void prlsd(const signed int *ls, size_t c, const char *del);

/** prints an array of ints, separated by ", " */
void prls(const int *ls, size_t c);

/** sums an array of ints */
__attribute__((pure, leaf))
int sumls(const signed int *ls, size_t c);


/** Prints an array of ints and (if needed) their sum. Prints a newline afterwards.
	@returns The sum
*/
int prSum(const signed int *ls, size_t c);

/** like strol, but with int instead of long */
signed int strtoi(const char *inp, char **end, int base);

/** like stroul, but with int instead of long */
signed int strtoui(const char *inp, char **end, int base);

/** Compares the dereferenced values of two int pointers, for qsort() */
__attribute__((pure, leaf))
int intpcomp(const void *l, const void *r);

/** The maximum of a and b */
__attribute__((const, leaf))
signed int max(signed int a, signed int b);

/** The minimum of a and b */
__attribute__((const, leaf))
signed int min(signed int a, signed int b);

/** like min(), but evaluates 0 as larger than every other value. */
__attribute__((const, leaf))
signed int min0(signed int a, signed int b);

/** the integral 𝜙(x) of a normal distribution with the given 𝜇 and 𝜎 */
__attribute__((const, leaf))
double phi(double x, double mu, double sigma);

/** The probability of x being rolled on a rounded normal distribution with the given 𝜇 and 𝜎 */
__attribute__((const, leaf))
double normal(double mu, double sigma, int x);


__attribute__((leaf, malloc, returns_nonnull, alloc_size(1)))
void *xmalloc(size_t siz);

__attribute__((leaf, malloc, returns_nonnull, alloc_size(1,2)))
void *xcalloc(size_t num, size_t siz);

__attribute__((leaf, malloc, returns_nonnull, alloc_size(2)))
void *xrealloc(void *ptr, size_t siz);

__attribute__((leaf, malloc, returns_nonnull, alloc_size(2,3)))
void *xrecalloc(void *ptr, size_t num, size_t siz);
