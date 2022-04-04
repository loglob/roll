/* represents probability functions that map N onto Q, with the sum of every value equaling 1. */
#pragma once
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "ranges.h"
#include "settings.h"

/* Represents a probability function that follows 4 axioms:
	(0) p ⊂ ℚ⁺∪{0}
	(1) ∑p = 1
	(2) p[0] > 0
	(3) p[len - 1] > 0 */
typedef struct prob
{
	// the lowest
	signed int low;
	// the length of p
	int len;
	// the probability values
	double *p;
} p_t;

/* The highest value of p */
#define p_h(p) ((p).low + (p).len - 1)

/* Stores the 'next' combination of n numbers between 0 and max (exclusive) in ind.
	returns false if the all-0-combination was returned, true otherwise. */
static bool combinations(int max, int n, int ind[n])
{
	int i;

	for (i = n - 1; i && ind[i] == ind[i-1]; i--)
		ind[i] = 0;

	ind[i]++;
	ind[i] %= max;

	return i || *ind;
}

/* Calculates the amount of distinct permutations of the given sorted list. */
static double permutations(int n, int ind[n])
{
	double nFact = 1;
	double div = 1;
	int runlen = 0;

	for (int i = 1; i < n; i++)
	{
		nFact *= i + 1;

		if(ind[i] == ind[i - 1])
			div *= ++runlen;
		else
		{
			div *= runlen + 1;
			runlen = 0;
		}
	}

	div *= runlen + 1;

	return nFact / div;
}

/* creates a buffer that maps i onto (N choose (i+1)) */
int *chooseBuf(int N)
{
	if(N < 2)
		return NULL;

	int *buf = xcalloc(N - 1, sizeof(int));
	int *cur = buf, *last = buf + (N / 2);

	if(N % 2 == 0)
	{
		int *swap = cur;
		cur = last;
		last = swap;
	}

	*last = 2;

	for (int n = 3; n <= N; n++)
	{
		cur[0] = last[0] + 1;

		for (int k = 1; k < n / 2; k++)
			cur[k] = last[k-1] + (k == (n-1)/2 ? last[k-1] : last[k]);

		int *swap = cur;
		cur = last;
		last = swap;
	}

	int m = (N-1)/2;
	for (int i = 0; i < m; i++)
		cur[i] = last[m - i - 1];

	return buf;
}

/* The probability of num in p */
double probof(struct prob p, signed int num)
{
	if(num >= p.low && num < p.low + p.len)
		return p.p[num - p.low];
	else
		return 0.0;
}

/* The probability of a roll being STRICTLY LESS THAN num in p */
double probof_lt(struct prob p, signed int num)
{
	if(num <= p.low)
		return 0;
	else if(num > p_h(p))
		return 1;
	// the value is somewhere in the range

	double sum = 0;

	for (int i = 0; i < (num - p.low); i++)
		sum += p.p[i];

	return sum;
}

/* Creates a uniform distribution of the range 1..n (inclusive) */
struct prob p_uniform(int n)
{
	assert(n > 0);
	double *p = xmalloc(n * sizeof(double));

	for (int i = 0; i < n; i++)
		p[i] = 1.0 / n;

	return (struct prob){
		.len = n,
		.low = 1,
		.p = p
	};
}

/* Generates the probability function of y = P(-x). In-place. */
struct prob p_negs(struct prob p)
{
	for (int i = 0; i < p.len / 2; i++)
	{
		double *l = &p.p[i], *r = &p.p[p.len - 1 - i];
		double v = *l;

		*l = *r;
		*r = v;
	}

	p.low = -(p.low + p.len - 1);
	return p;
}

/* Clones the given probability function. Exits on malloc failure. */
struct prob p_dup(struct prob p)
{
	double *pp = xmalloc(p.len * sizeof(double));
	memcpy(pp, p.p, p.len * sizeof(double));

	return (struct prob){
		.len = p.len,
		.low = p.low,
		.p = pp
	};
}

/* Creates a probability function that maps the given value to 1. */
struct prob p_constant(int val)
{
	double *p = xmalloc(sizeof(double));

	*p = 1.0;

	return (struct prob){
		.len = 1,
		.low = val,
		.p = p
	};
}

/* Frees a probability function. */
void p_free(struct prob p)
{
	free(p.p);
}

/* Emulates rolling on p and rerolling once if the value is in rr. In-place. */
struct prob p_rerolls(struct prob p, int n, int rr[n])
{
	if(!rr)
		return p;

	double prr = 0.0;

	for (int i = 0; i < p.len; i++)
	{
		if(in(n, rr, p.low + i))
			prr += p.p[i];
	}

	if(prr == 0.0 || prr == 1.0)
		return p;

	for (int i = 0; i < p.len; i++)
		p.p[i] *= !in(n, rr, p.low + i) + prr;

	return p;
}

/* Like p_rerolls, with unlimited rerolls. */
struct prob p_sans(struct prob p, int n, int rr[n])
{
	if(!rr)
		return p;

	double prr = 0.0;
	int start = 0, end = 0;

	for (int i = 0; i < p.len; i++)
	{
		if(in(n, rr, p.low + i))
		{
			if(i == start)
				start++;

			prr += p.p[i];
		}
		else
			end = p.len - i - 1;
	}

	if(prr == 0.0)
		return p;
	if(prr == 1.0)
		eprintf("Every case of the function is discarded.\n");

	for (int i = 0; i < p.len; i++)
		p.p[i] *= !in(n, rr, p.low + i) / (1 - prr);

	if(start)
	{
		p.low += start;
		memmove(p.p, p.p + start, (p.len - start) * sizeof(double));
		end += start;
	}

	if(end)
	{
		p.len -= end;
		p.p = realloc(p.p, p.len * sizeof(double));
	}

	return p;
}

/* Emulates rolling on l and r, then adding the results. */
struct prob p_add(struct prob l, struct prob r)
{
	int high = l.low + r.low + l.len + r.len - 2;
	int low = l.low + r.low;
	int len = high - low + 1;

	double *p = xcalloc(len, sizeof(double));

	for (int i = 0; i < l.len; i++)
		for (int j = 0; j < r.len; j++)
			p[i + j] += l.p[i] * r.p[j];

	return (struct prob){
		.len = len,
		.low = low,
		.p = p
	};
}

/* Like p_add. In-place. */
struct prob p_adds(struct prob l, struct prob r)
{
	struct prob m = p_add(l, r);

	p_free(l);

	if(l.p != r.p)
		p_free(r);

	return m;
}

/* Emulates rolling on l and r, then multiplying the results. */
struct prob p_cmul(struct prob l, struct prob r)
{
	rl_t res = range_mul(l.low, p_h(l), r.low, p_h(r));
	double *p = xcalloc(res.len, sizeof(double));

	for (int i = 0; i < l.len; i++)
		for (int j = 0; j < r.len; j++)
			p[(i + l.low) * (j + r.low) - res.low] += l.p[i] * r.p[j];

	return (struct prob){ .low = res.low, .len = res.len, .p = p };
}

/* Like p_cmul(), in-place. */
struct prob p_cmuls(struct prob l, struct prob r)
{
	struct prob v = p_cmul(l, r);
	p_free(l);

	if(l.p != r.p)
		p_free(r);

	return v;
}

/* l/r. Division by 0 is discarded. */
struct prob p_cdiv(struct prob l, struct prob r)
{
	if(r.len == 1 && r.low == 0)
		eprintf("Division by constant 0.\n");

	rl_t res = range_div(l.low, p_h(l), r.low, p_h(r));
	int len = res.high - res.low + 1;

	double *p = xcalloc(len, sizeof(double));
	double discarded = 0;

	for (int ri = 0; ri < r.len; ri++)
	{
		if(ri + r.low == 0)
			discarded = r.p[ri];
		else for (int li = 0; li < l.len; li++)
			p[(li + l.low) / (ri + r.low) - res.low] += l.p[li] * r.p[ri];
	}

	// Restore axiom (1)
	if(discarded > 0)
	{
		for (int i = 0; i < len; i++)
			p[i] /= 1.0 - discarded;
	}

	return (struct prob){ .low = res.low, .len = len, .p = p };
}

/* Internal implementation of p_mul. Inconsistent on being in-place or not. */
struct prob _p_mulk(struct prob p, signed int x)
{
	if(x == 0)
		return p_constant(0);
	if(x == 1)
		return p;
	if(x == 2)
		return p_add(p, p);
	if(x < 0)
		return p_negs(_p_mulk(p, -x));

	struct prob v = _p_mulk(p, x/2);
	v = ((v.p == p.p) ? p_add : p_adds)(v, v);

	if(x % 2)
	{
		struct prob _v = v;
		v = p_add(v, p);
		p_free(_v);
	}

	return v;
}

/* Emulates rolling x times on p and adding the results. */
struct prob p_mulk(struct prob p, signed int x)
{
	struct prob _p = p_dup(p);
	struct prob r = _p_mulk(_p, x);

	if(r.p != _p.p)
		p_free(_p);

	return r;
}

/* Like p_mul, in-place. */
struct prob p_mulks(struct prob p, signed int x)
{
	struct prob r = _p_mulk(p, x);

	if(r.p != p.p)
		p_free(p);

	return r;
}

/* adds l onto r*q. Leaves l and r unchanged */
struct prob p_merge(struct prob l, struct prob r, double q)
{
	assert(q > 0);

	int low = min(l.low, r.low);
	int high = max(l.low + l.len, r.low + r.len) - 1;
	int len = high - low + 1;

	double *p = xcalloc(len, sizeof(double));

	for (int i = 0; i < l.len; i++)
		p[i + l.low - low] = l.p[i];
	for (int i = 0; i < r.len; i++)
		p[i + r.low - low] += r.p[i] * q;

	return (struct prob){ .low = low, .len = len, .p = p };
}

/* adds l onto r*q. In-place. */
struct prob p_merges(struct prob l, struct prob r, double q)
{
	p_t res = p_merge(l, r, q);
	p_free(l);

	if(l.p != r.p)
		p_free(r);

	return res;
}

/* Rolls on l and sums that many rolls of r. */
struct prob p_muls(struct prob l, struct prob r)
{
	struct prob sum = { };

	for (int i = 0; i < l.len; i++)
	{
		struct prob cur = p_mulk(r, i + l.low);
		sum = p_merges(i ? sum : ((struct prob){ .low = cur.low }), cur, l.p[i]);
	}

	p_free(l);

	if(l.p != r.p)
		p_free(r);

	return sum;
}

/* Emulates rolling on p of times, and then selecting the highest/lowest value. */
struct prob p_selectOne(struct prob p, int of, bool selHigh)
{
	// sum[n] ::= Sum(p.p[i] | i <= n)
	assert(of >= 2);
	int *c = chooseBuf(of);
	#define choose(N, k) ((k <= 0 || k >= N) ? 1 : c[k-1])

	double *sum = xcalloc(p.len, sizeof(double));
	sum[0] = p.p[0];

	for (int i = 1; i < p.len; i++)
		sum[i] = p.p[i] + sum[i - 1];

	for (int i = 0; i < p.len; i++)
	{
		// probability of a value being smaller / larger than i.
		double plti = selHigh ? (i ? sum[i - 1] : 0.0) : (1.0 - sum[i]);
		double px = pow(p.p[i], of);

		for (int j = 1; j < of; j++)
			px += pow(p.p[i], j) * pow(plti, of - j) * c[j-1];

		p.p[i] = px;
	}

	free(sum);
	free(c);

	return p;

}

/* Emulates rolling on p of times, then adding the sel highest/lowest rolls. */
struct prob p_select(struct prob p, int sel, int of, bool selHigh)
{
	assert(sel < of);

	// Use the MUCH faster selectOne algorithm (O(n) vs O(n!)
	if(sel == 1)
		return p_selectOne(p, of, selHigh);

	int *v = xcalloc(of, sizeof(int));
	struct prob c = (struct prob){ .len = (p.len-1) * sel + 1, .low = sel };
	c.p = xcalloc(c.len, sizeof(double));

	do
	{
		double q = permutations(of, v);
		int sum = 0;

		for (int i = 0; i < of; i++)
		{
			q *= p.p[v[i]];

			if(selHigh ? (i < sel) : (i >= of - sel))
				sum += v[i];
		}

		c.p[sum] += q;
	} while(combinations(p.len, of, v));

	free(v);
	return c;
}

/* Cuts l values from the left and r values from the right of the given probability array.
	Restores axioms (2) and (3), ignoring (1). */
struct prob p_cuts(struct prob p, int l, int r)
{
	for (; p.p[l] <= 0.0; l++)
		;
	for (; p.p[p.len - r - 1] <= 0.0; r++)
		;

	p.len -= l + r;
	p.low += l;

	assert(p.len > 0);
	memmove(p.p, p.p + l, p.len);
	p.p = realloc(p.p, p.len);

	return p;
}

/* stack-allocated p_constant() */
#define P_CONST(x) (struct prob){ .low = x, .len = 1, .p = (double[]){ 1.0 } }

struct prob p_explodes(struct prob p)
{
	assert(p.len > 1);

	struct prob exp = p_add(p, P_CONST(p.low + p.len - 1));
	struct prob imp = p_adds(p_constant(p.low), p_negs(p_dup(p)));
	double Pmin = p.p[0];
	double Pmax = p.p[p.len - 1];

	// trim out min & max
	p.len -= 2;
	p.low++;
	memmove(p.p, p.p + 1, p.len * sizeof(double));
	p.p = xrealloc(p.p, p.len * sizeof(double));

	return p_merges(p_merges(p, exp, Pmax), imp, Pmin);
}

struct prob p_bool(double prob)
{
	if(prob == 0)
		return p_constant(0);
	else if(prob == 1)
		return p_constant(1);
	else
	{
		struct prob p = (struct prob){ .len = 2, .low = 0, .p = xmalloc(2 * sizeof(double)) };
		p.p[1] = prob;
		p.p[0] = 1 - p.p[1];
		return p;
	}
}

/* P(l < r) */
struct prob p_less(struct prob l, struct prob r)
{
	double prob = 0.0;

	for (int i = 0; i < l.len; i++)
	{
		// P(l > r[i])
		double pgt = 0;
		// equivalent index in r
		int j = i + 1 + l.low - r.low;

		for (; j < r.len; j++)
		{
			if(j >= 0)
				pgt += r.p[j];
		}

		prob += l.p[i] * pgt;
	}

	return p_bool(prob);
}

/** Determines P(x > 0) */
double p_true(struct prob x)
{
	double prob = 0;

	for (int i = (x.low <= 0) ? -x.low+1 : 0; i < x.len; i++)
		prob += x.p[i];

	return prob;
}

struct prob p_coalesces(struct prob l, struct prob r)
{
	if(l.low > 0)
	{
		p_free(r);
		return l;
	}

	int i = -l.low + 1;

	if(i >= l.len)
	{
		p_free(l);
		return r;
	}

	double p = p_true(l);
	return p_merges(p_cuts(l, i, 0), r, 1 - p);
}

/** Multiplies every probability of p by k.
 * 	Ignores axiom (1).
 */
struct prob p_scales(struct prob p, double k)
{
	assert(k > 0);

	for (int i = 0; i < p.len; i++)
		p.p[i] *= k;

	return p;
}

struct prob p_terns(struct prob cond, struct prob then, struct prob otherwise)
{
	double p = p_true(cond);
	p_free(cond);

	if(p == 0.0)
	{
		p_free(then);
		return otherwise;
	}
	if(p == 1.0)
	{
		p_free(otherwise);
		return then;
	}

	return p_merges(p_scales(then, p), otherwise, 1 - p);
}


/* Simulates n rounds of exploding-only rolls on p. In-place. */
struct prob p_explode_ns(const struct prob p, int n)
{
	assert(p.len > 1);
	assert(n > 0);

	int max = p.len + p.low - 1;
	double pmax = p.p[p.len - 1];

	// Axiom (1) gets restored over the loop
	p_t res = p_dup((struct prob){ .len = p.len - 1, .low = p.low, .p = p.p });
	double pcur = pmax;

	for (int i = 1; i < n; i++, pcur *= pmax)
	{
		p_t cur = p_merge(res, (struct prob){ .len = p.len - 1, .low = p.low + max * i, .p = p.p }, pcur);
		p_free(res);
		res = cur;
	}

	// final round without cutting off the maximum. Effectively cut off the converging infinite series, restoring Axiom (1)
	return p_merges(res, (struct prob){ .len = p.len, .low = p.low + max * n, .p = p.p }, pcur);
}

/** Emulates rolling on l and r, then selecting the higher value */
struct prob p_max(struct prob l, struct prob r)
{
	struct prob res;
	res.low = max(l.low, r.low);
	res.len = max(p_h(l), p_h(r)) - res.low + 1;
	res.p = xcalloc(res.len, sizeof(double));

	// very naive algo, can't think of smart implementation right now (it's 4AM)
	for (int i = 0; i < res.len; i++)
	{
		int n = i + res.low;
		double pl = probof(l, n), pr = probof(r, n);

		res.p[i] = (pl > 0 ? pl * probof_lt(r,n) : 0) + (pr ? pr * probof_lt(l,n) : 0) + pl * pr;
	}

	// I think Axioms 2 & 3 must always hold, not 100% though
	return res;
}