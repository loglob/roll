#pragma once
#include <stddef.h>

/** Marks a function that never returns an aliased pointer, if such an attribute is supported. */
#define MALLOC_ATTR
/** Marks a function that never returns a NULL pointer, if such an attribute is supported. */
#define RET_NONNULL_ATTR
/** Marks a function that always returns and never invokes callbacks or longjumps, if such an attribute is supported */
#define LEAF_ATTR
/** Marks a function with no effects besides returning a value, if such an attribute is supported */
#define PURE_ATTR
/** Marks a pure function that doesn't dereference any pointers or use global variables, if such an attribute is supported */
#define CONST_ATTR
/** Marks a function that never returns, if such an attribute is supported */
#define NORETURN_ATTR
/** Marks a function that takes a printf format string as well as variadic arguments for it, if such an attribute is supported */
#define PRINTF_ATTR(fmtArg, paramStartArg)
/** Marks a function that returns allocated memory of specified size, if such an attribute is supported */
#define ALLOC_SIZE_ATTR(sizArg, ...)
/** Marks a function or argument that is never used, if such an attribute is supported */
#define UNUSED_ATTR

#ifdef __has_attribute
# if __has_attribute(malloc)
#  undef  MALLOC_ATTR
#  define MALLOC_ATTR __attribute__((malloc))
# endif
# if __has_attribute(returns_nonnull)
#  undef  RET_NONNULL_ATTR
#  define RET_NONNULL_ATTR __attribute__((returns_nonnull))
# endif
# if __has_attribute(leaf)
#  undef  LEAF_ATTR
#  define LEAF_ATTR __attribute__((leaf))
# endif
# if __has_attribute(pure)
#  undef  PURE_ATTR
#  define PURE_ATTR __attribute__((pure))
# endif
# if __has_attribute(const)
#  undef  CONST_ATTR
#  define CONST_ATTR __attribute__((const))
# endif
# if __has_attribute(noreturn)
#  undef  NORETURN_ATTR
#  define NORETURN_ATTR __attribute__((noreturn))
# endif
# if __has_attribute(format)
#  undef  PRINTF_ATTR
#  define PRINTF_ATTR(fmt,par) __attribute__((format(printf, fmt, par)))
# endif
# if __has_attribute(alloc_size)
#  undef  ALLOC_SIZE_ATTR
#  define ALLOC_SIZE_ATTR(sizArg, ...) __attribute__((alloc_size(sizArg ,##__VA_ARGS__)))
# endif
# if __has_attribute(unused)
#  undef  UNUSED_ATTR
#  define UNUSED_ATTR __attribute__((unused))
# endif
#endif


#define CLONE_SIG(func, type) MALLOC_ATTR RET_NONNULL_ATTR LEAF_ATTR type *func(type x);
#define CLONE(func, type) type *func(type x) { type *y = xmalloc(sizeof(x)); *y = x; return y; }
#define FREE_SIG(xfree, type) void xfree##P;
#define FREE_P(xfree, type) void xfree##P (type *x) { if(x) { xfree(*x); free(x); } }

/** like printf, but prints onto stderr and exits with EXIT_FAILURE. */
NORETURN_ATTR
PRINTF_ATTR(1, 2)
void eprintf(const char *fmt, ...);

/** prints an array of ints with a custom delimiter */
void prlsd(const signed int *ls, size_t c, const char *del);

/** prints an array of ints, separated by ", " */
void prls(const int *ls, size_t c);

/** sums an array of ints */
PURE_ATTR LEAF_ATTR
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
PURE_ATTR LEAF_ATTR
int intpcomp(const void *l, const void *r);

/** The maximum of a and b */
CONST_ATTR LEAF_ATTR
signed int max(signed int a, signed int b);

/** The minimum of a and b */
CONST_ATTR LEAF_ATTR
signed int min(signed int a, signed int b);

/** like min(), but evaluates 0 as larger than every other value. */
CONST_ATTR LEAF_ATTR
signed int min0(signed int a, signed int b);

/** the integral 𝜙(x) of a normal distribution with the given 𝜇 and 𝜎 */
CONST_ATTR LEAF_ATTR
double phi(double x, double mu, double sigma);

/** The probability of x being rolled on a rounded normal distribution with the given 𝜇 and 𝜎 */
CONST_ATTR LEAF_ATTR
double normal(double mu, double sigma, int x);

LEAF_ATTR
MALLOC_ATTR
RET_NONNULL_ATTR
ALLOC_SIZE_ATTR(1)
void *xmalloc(size_t siz);

LEAF_ATTR
MALLOC_ATTR
RET_NONNULL_ATTR
ALLOC_SIZE_ATTR(1,2)
void *xcalloc(size_t num, size_t siz);

LEAF_ATTR
MALLOC_ATTR
RET_NONNULL_ATTR
ALLOC_SIZE_ATTR(2)
void *xrealloc(void *ptr, size_t siz);

LEAF_ATTR
MALLOC_ATTR
RET_NONNULL_ATTR
ALLOC_SIZE_ATTR(2,3)
void *xrecalloc(void *ptr, size_t num, size_t siz);
