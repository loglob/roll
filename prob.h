/* represents probability functions that map N onto Q, with the sum of every value equaling 1. */
#pragma once
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "ranges.h"
#include "settings.h"
#include "set.h"

/* Represents a probability function that follows 4 axioms:
	(0) p ⊂ ℚ⁺∪{0}
	(1) ∑p = 1
	(2) p[0] > 0
	(3) p[len - 1] > 0 */
struct prob
{
	// the lowest
	signed int low;
	// the length of p
	int len;
	// the probability values
	double *p;
};

/* The highest value of p */
#define p_h(p) ((p).low + (p).len - 1)

/** Stores the 'next' combination of n numbers between 0 and max (exclusive) in ind.
	@param max The amount of numbers to choose from
	@param n The amount of choices to make, i.e. the number of entries in ind
	@param ind A buffer ∈ [0;max]ⁿ containing the last permutation, which is overwritten with the next one
	@returns false if the all-0-combination was returned, true otherwise.
*/
static bool combinations(int max, int n, int ind[n])
{
	int i;

	for (i = n - 1; i && ind[i] == ind[i-1]; i--)
		ind[i] = 0;

	ind[i]++;
	ind[i] %= max;

	return i || *ind;
}

/** Calculates the total number of permutations of a list
	@param n The length of ind
	@param ind A sorted list of possible choices
	@return The amount of distinct permutations there are of ind
*/
static double permutations(int n, int ind[n])
{
	double nFact = 1;
	double div = 1;
	int runLen = 0;

	for (int i = 1; i < n; i++)
	{
		nFact *= i + 1;

		if(ind[i] == ind[i - 1])
			div *= ++runLen;
		else
		{
			div *= runLen + 1;
			runLen = 0;
		}
	}

	div *= runLen + 1;

	return nFact / div;
}

/** Computes all binomial coefficients for a given number
	@param N The n parameter for the binomial coefficients
	@returns Array a of length N-1, s.t. a[i] = (N choose (i+1)), or NULL if N <= 1
*/
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

/** The probability of a result in p
	@param p A probability function
	@param num A result
	@returns P(x from p = num), i.e. p(num)
*/
double probof(struct prob p, signed int num)
{
	if(num >= p.low && num < p.low + p.len)
		return p.p[num - p.low];
	else
		return 0.0;
}

/* Creates a uniform distribution of the range 1..n (inclusive), or n..-1 if n < 0. */
struct prob p_uniform(int n)
{
	assert(n != 0);
	int l = abs(n);
	double *p = xmalloc(l * sizeof(double));

	for (int i = 0; i < l; i++)
		p[i] = 1.0 / l;

	return (struct prob){
		.len = l,
		.low = n < 0 ? n : 1,
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

	p.low = -p_h(p);
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

/* Emulates rolling on p and rerolling once if the value is in the given signed set. In-place. */
struct prob p_rerolls(struct prob p, struct set set)
{
	if(set_hasAll(set, p.low, p_h(p)) || !set_hasAny(set, p.low, p_h(p)))
		return p;

	double prr = 0.0;

	for (int i = 0; i < p.len; i++)
	{
		if(set_has(set, p.low + i))
			prr += p.p[i];
	}

	if(prr == 0.0 || prr == 1.0)
		return p;

	for (int i = 0; i < p.len; i++)
		p.p[i] *= !set_has(set, p.low + i) + prr;

	return p;
}

/* Like p_rerolls, with unlimited rerolls. */
struct prob p_sans(struct prob p, struct set set)
{
	if(!set_hasAny(set, p.low, p_h(p)))
	// no rerolls can take place
		return p;
	if(set_hasAll(set, p.low, p_h(p)))
	// only rerolls can take place
		eprintf("Every case of the function is discarded.\n");

	double prr = 0.0;
	int start = 0, end = 0;

	for (int i = 0; i < p.len; i++)
	{
		if(set_has(set, p.low + i))
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
		p.p[i] *= !set_has(set, p.low + i) / (1 - prr);

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
	rl_t res = range_mul(range_lim(l.low, p_h(l)), range_lim(r.low, p_h(r)));
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

	rl_t res = range_div(range_lim(l.low, p_h(l)), range_lim(r.low, p_h(r)));
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
	struct prob res = p_merge(l, r, q);
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

/* Emulates rolling on p of times, and then selecting the highest/lowest value. In-place. */
struct prob p_selectOne(struct prob p, int of, bool selHigh)
{
	assert(of > 0);

	if(of == 1)
		return p;

	int *c = chooseBuf(of);
	#define choose(N, k) ((k <= 0 || k >= N) ? 1 : c[k-1])

	// sum[n] := Sum(p.p[i] | i <= n)
	double *sum = xcalloc(p.len, sizeof(double));
	sum[0] = p.p[0];

	for (int i = 1; i < p.len; i++)
		sum[i] = p.p[i] + sum[i - 1];

	for (int i = 0; i < p.len; i++)
	{
		// probability of a value being smaller / larger than i.
		double pLtI = selHigh ? (i ? sum[i - 1] : 0.0) : (1.0 - sum[i]);
		double px = pow(p.p[i], of);

		for (int j = 1; j < of; j++)
			px += pow(p.p[i], j) * pow(pLtI, of - j) * c[j-1];

		p.p[i] = px;
	}

	free(sum);
	free(c);

	return p;
	#undef choose
}

/* Like p_selectOne with selHigh=true, but 'goes bust' if majority of rolls are lowest. Leaves p intact. */
struct prob p_selectOne_bust(struct prob p, int sel, int of)
{
	assert(p.len);

	int *choose = chooseBuf(of);
	// p without 1s
	struct prob p2 = p_sans(p_dup(p), SINGLETON(p.low, p.low));
	struct prob total = p_constant(p.low - 1);

	// range over # of 1s
	for (int n = 0; n < sel; n++)
	{
		// prob. of rolling n 1s
		double p_n = pow(*p.p, n) * pow(1 - *p.p, of - n) * (n ? choose[n-1] : 1);

		// value distribution for that amount of 1s
		struct prob vals = p_selectOne(p_dup(p2), of - n, true);

		total = p_merges(total, vals, p_n);
		// keep track of covered cases
		total.p[0] -= p_n;
	}

	p_free(p2);
	free(choose);

	return total;
}

/* Emulates rolling on p of times, then adding the sel highest/lowest rolls. */
struct prob p_select(struct prob p, int sel, int of, bool selHigh)
{
	// Use the MUCH faster selectOne algorithm (O(n) vs O(n!)
	if(sel == 1)
		return p_selectOne(p, of, selHigh);
	if(sel == of)
		return p_mulk(p, sel);

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

/* Simulates rolling on p, adding another roll to the maximum result and subtracting another roll from the minimum result. In-place. */
struct prob p_explodes(struct prob p)
{
	assert(p.len > 1);

	struct prob exp = p_add(p, P_CONST(p_h(p)));
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

/* Creates a probability distribution such that P(x=1) = prob, and P(x=0) = 1-prob  */
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

/* P(x >= k) */
double p_geqK(struct prob x, int k)
{
	double pgt = 0;

	// start at equivalent index
	for (int j = max(0, k - x.low); j < x.len; j++)
		pgt += x.p[j];

	return pgt;
}

/* P(l <= r) */
double p_leq(struct prob l, struct prob r)
{
	double prob = 0.0;

	for (int i = 0; i < l.len; i++)
		prob += l.p[i] * p_geqK(r, l.low + i);

	return prob;
}

double p_eq(struct prob l, struct prob r)
{
	double prob = 0.0;

	for (int i = 0; i < l.len; i++)
		prob += l.p[i] * probof(r, i + l.low);

	return prob;
}

/* P(x > 0) */
double p_true(struct prob x)
{
	double prob = 0;

	for (int i = (x.low <= 0) ? -x.low+1 : 0; i < x.len; i++)
		prob += x.p[i];

	return prob;
}

/* Simulates rolling on l, then replacing any values <= 0 with a roll on r. In-place. */
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

/* Multiplies every probability of p by k. In-place.
	Ignores axiom (1).
 */
struct prob p_scales(struct prob p, double k)
{
	assert(k > 0);

	for (int i = 0; i < p.len; i++)
		p.p[i] *= k;

	return p;
}

/* Simulates rolling on cond, returning 'then' if the result is > 0 and 'otherwise' otherwise. In-place. */
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
	double pMax = p.p[p.len - 1];

	// Axiom (1) gets restored over the loop
	struct prob res = p_dup((struct prob){ .len = p.len - 1, .low = p.low, .p = p.p });
	double pCur = pMax;

	for (int i = 1; i < n; i++, pCur *= pMax)
	{
		struct prob cur = p_merge(res, (struct prob){ .len = p.len - 1, .low = p.low + max * i, .p = p.p }, pCur);
		p_free(res);
		res = cur;
	}

	// final round without cutting off the maximum. Effectively cut off the converging infinite series, restoring Axiom (1)
	return p_merges(res, (struct prob){ .len = p.len, .low = p.low + max * n, .p = p.p }, pCur);
}

/* Emulates rolling on l and r, then selecting the higher value. In-place. */
struct prob p_maxs(struct prob l, struct prob r)
{
	struct prob res;
	res.low = max(l.low, r.low);
	res.len = max(p_h(l), p_h(r)) - res.low + 1;
	res.p = xcalloc(res.len, sizeof(double));

	// probability of l/r being less than the current n
	double l_lt = 0, r_lt = 0;

	for(signed int n = l.low; n < res.low; n++)
		l_lt += probof(l, n);
	for(signed int n = r.low; n < res.low; n++)
		r_lt += probof(r, n);

	for (int i = 0; i < res.len; i++)
	{
		int n = i + res.low;
		double pl = probof(l, n), pr = probof(r, n);

		res.p[i] = pl * r_lt + pr * l_lt + pl * pr;

		l_lt += pl;
		r_lt += pr;
	}

	p_free(l);

	if(r.p != l.p)
		p_free(r);

	// I think Axioms 2 & 3 must always hold, not 100% though
	return res;
}

/* Emulates rolling on l and r, then selecting the lower value. In-place. */
struct prob p_mins(struct prob l, struct prob r)
{
	struct prob res;
	res.low = min(l.low, r.low);
	res.len = min(p_h(l), p_h(r)) - res.low + 1;
	res.p = xcalloc(res.len, sizeof(double));

	// probability of l/r <= n
	double l_lte = 0, r_lte = 0;

	for(signed int n = l.low; n < res.low; n++)
		l_lte += probof(l, n);
	for(signed int n = r.low; n < res.low; n++)
		r_lte += probof(r, n);

	for (int i = 0; i < res.len; i++)
	{
		int n = i + res.low;
		double pl = probof(l, n), pr = probof(r, n);

		l_lte += pl;
		r_lte += pr;

		// !(x <= n) <=> x>n
		res.p[i] = pl * pr + pl * (1 - r_lte) + pr * (1 - l_lte);
	}

	p_free(l);

	if(r.p != l.p)
		p_free(r);

	return res;
}

/* Emulates rolling on p, then rolling a fair die with that many pips */
struct prob p_dies(struct prob p)
{
	assert(p.len >= 1);

	struct prob sum = p_scales(p_uniform(p.low), p.p[0]);

	for (int i = 1; i < p.len; i++)
	{
		if(p.p[1] > 0)
			sum = p_merges(sum, p_uniform(p.low + i), p.p[1]);
	}

	return sum;
}
