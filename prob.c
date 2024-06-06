/* represents probability functions that map N onto Q, with the sum of every value equaling 1.
	any function ending on 's' acts in-place or frees its arguments after use. */
#include "ast.h"
#include "parse.h"
#include "prob.h"
#include "set.h"
#include "util.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define CLEAN_BIOP(T, name) T name##s (struct Prob l, struct Prob r) {\
	T res = name(l,r); p_free(l); if(l.p != r.p) p_free(r); return res; }

/* The highest value of p */
#define p_h(p) ((p).low + (p).len - 1)

/** Stores the 'next' combination of n numbers between 0 and max (exclusive) in ind.
	Always returns distinct, sorted (descending) arrays.
	Iterates in ascending lexicographic order.
	@param max The amount of numbers to choose from, i.e. the exclusive upper bound
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
static int *chooseBuf(int N)
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

double pt_hit(struct PatternProb pt, int v)
{
	if(!pt.op)
		return set_has(pt.set, v) ? 1.0 : 0.0;

	const struct Prob q = P_CONST(v);

	switch(pt.op)
	{
		case GT_EQ:
			return p_leq(pt.prob, q);
		case '<':
			return 1.0 - p_leq(pt.prob, q);

		case LT_EQ:
			return p_leq(q, pt.prob);
		case '>':
			return 1.0 - p_leq(q, pt.prob);

		case '=':
			return p_eq(q, pt.prob);
		case NEQ:
			return 1.0 - p_eq(q, pt.prob);

		default:
			eprintf("Invalid pattern: Unknown relational operator: %s\n", tkstr(pt.op));
			__builtin_unreachable();
	}
}

double p_sum(struct Prob p)
{
	double sum = 0;

	for(int i = 0; i < p.len; ++i)
		sum += p.p[i];

	return sum;
}

/** The total sum of a probability function.
	i.e. 1 is axiom (1) holds. */
double p_norms(struct Prob *p)
{
	double sum = p_sum(*p);

	if(sum != 1.0 && sum != 0.0) 
	{
		for(int i = 0; i < p->len; ++i)
			p->p[i] /= sum;
	}

	return sum;
}

struct Prob pt_probs(struct PatternProb pt, struct Prob *p)
{
	struct Prob q = {
		.low = p->low,
		.len = p->len,
		.p = malloc(p->len * sizeof(double))
		};

	for(int i = 0; i < p->len; ++i)
	{
		double pHit = pt_hit(pt, p->low + i);

		q.p[i] = p->p[i] * pHit;
		p->p[i] *= 1.0 - pHit;
	}
	
	q = p_cuts(q, 0, 0);
	*p = p_cuts(*p, 0, 0);

	return q;

}

double probof(struct Prob p, signed int num)
{
	if(num >= p.low && num < p.low + p.len)
		return p.p[num - p.low];
	else
		return 0.0;
}

/* Creates a uniform distribution of the range 1..n (inclusive), or n..-1 if n < 0. */
struct Prob p_uniform(int n)
{
	assert(n != 0);
	int l = abs(n);
	double *p = xmalloc(l * sizeof(double));

	for (int i = 0; i < l; i++)
		p[i] = 1.0 / l;

	return (struct Prob){
		.len = l,
		.low = n < 0 ? n : 1,
		.p = p
	};
}

struct Prob p_negs(struct Prob p)
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

struct Prob p_dup(struct Prob p)
{
	double *pp = xmalloc(p.len * sizeof(double));
	memcpy(pp, p.p, p.len * sizeof(double));

	return (struct Prob){
		.len = p.len,
		.low = p.low,
		.p = pp
	};
}

struct Prob p_constant(int val)
{
	double *p = xmalloc(sizeof(double));

	*p = 1.0;

	return (struct Prob){
		.len = 1,
		.low = val,
		.p = p
	};
}

void p_free(struct Prob p)
{
	free(p.p);
}

void pp_free(struct PatternProb pp)
{
	if(pp.op)
		p_free(pp.prob);
	// set is aliased in AST
}

double p_has(struct Prob p, struct Set set)
{
	if(!set_hasAny(set, p.low, p_h(p)))
		return 0.0;
	if(set_hasAll(set, p.low, p_h(p)))
		return 1.0;

	double prr = 0.0;

	for (int i = 0; i < p.len; i++)
	{
		if(set_has(set, p.low + i))
			prr += p.p[i];
	}

	return prr;
}

struct Prob p_rerolls(struct Prob p, struct Set set)
{
	double prr = p_has(p, set);

	if(prr == 0.0 || prr == 1.0)
		return p;

	for (int i = 0; i < p.len; i++)
		p.p[i] *= !set_has(set, p.low + i) + prr;

	return p;
}

struct Prob p_sans(struct Prob p, struct Set set)
{
	double prr = p_has(p, set);
	int start = 0, end = 0;

	if(prr == 0.0)
		return p;
	if(prr == 1.0)
		eprintf("Every case of the function is discarded.\n");

	for (int i = 0; i < p.len; i++)
	{
		if(set_has(set, p.low + i))
		{
			if(i == start)
				start++;

			p.p[i] = 0;
		}
		else
		{
			p.p[i] *= 1.0 / (1.0 - prr);
			end = p.len - i - 1;
		}
	}

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

struct Prob p_add(struct Prob l, struct Prob r)
{
	int high = l.low + r.low + l.len + r.len - 2;
	int low = l.low + r.low;
	int len = high - low + 1;

	double *p = xcalloc(len, sizeof(double));

	for (int i = 0; i < l.len; i++)
		for (int j = 0; j < r.len; j++)
			p[i + j] += l.p[i] * r.p[j];

	return (struct Prob){
		.len = len,
		.low = low,
		.p = p
	};
}

CLEAN_BIOP(struct Prob, p_add)

/** @return The smallest negative (farthest to 0) possible value in p */
static inline signed int negMin(struct Prob p)
{
	return p.low >= 0 ? 0 : p.low;
}

/** @return The largest negative (closest to 0) possible value in p */
static inline signed int negMax(struct Prob p)
{
	int h = p_h(p);

	if(h < 0)
		return h;

	if(p.low >= 0)
		return 0;

	for(int i = -1; i > p.low; --i)
	{
		if(probof(p, i) > 0)
			return i;
	}

	return p.low;
}

/** @return The smallest positive (closest to 0) possible value in p */
static inline signed int posMin(struct Prob p)
{
	if(p.low > 0)
		return p.low;
	
	int h = p_h(p);

	if(h <= 0)
		return 0;

	for(int i = 1; i < h; ++i)
	{
		if(probof(p, i) > 0)
			return i;
	}

	return h;
}

/** @return The largest positive (farthest from 0) possible value in p */
static inline signed int posMax(struct Prob p)
{
	int h = p_h(p);
	return h <= 0 ? 0 : h;
}

struct Prob p_cmul(struct Prob l, struct Prob r)
{
	bool lZ = probof(l, 0) > 0, rZ = probof(r, 0) > 0;
	int lNL = negMin(l), lNH = negMax(l), lPL = posMin(l), lPH = posMax(l);
	int rNL = negMin(r), rNH = negMax(r), rPL = posMin(r), rPH = posMax(r);

	assert(lZ || lNL || lPH);
	assert(rZ || rNL || rPH);

	int lo = min(lNL * rPH, lPH * rNL);

	if(lo == 0) // must be positive
		lo = min(lPL * rPL, lNH * rNH);
	
	int hi = max(lPH * rPH, lNL * rNL);

	if(hi == 0 && !lZ && !rZ) // must be negative
	{
		int x = lNH * rPL, y = lPL * rNH;

		if(x && y)
			hi = max(x, y);
		else if(x)
			hi = x;
		else
			hi = y;
	}

	assert(hi >= lo);
	int len = hi - lo + 1;
	
	double *p = xcalloc(len, sizeof(double));

	for (int i = 0; i < l.len; i++)
		for (int j = 0; j < r.len; j++)
			p[(i + l.low) * (j + r.low) - lo] += l.p[i] * r.p[j];

	return (struct Prob){ .low = lo, .len = len, .p = p };
}

CLEAN_BIOP(struct Prob, p_cmul)

struct Prob p_cdiv(struct Prob l, struct Prob r)
{
	bool lZ = probof(l, 0) > 0;
	int lNL = negMin(l), lNH = negMax(l), lPL = posMin(l), lPH = posMax(l);
	int rNL = negMin(r), rNH = negMax(r), rPL = posMin(r), rPH = posMax(r);

	assert(lZ || lNL || lPH);
	
	if(!rNL && !rPH)
		eprintf("Division by constant 0.\n");

	int lo = lZ ? 0 : INT_MAX;

	if(rPL)
		lo = min(lo, lNL / rPL);
	if(rNH)
		lo = min(lo, lPH / rNH);
	if(rPH)
		lo = min(lo, lPL / rPH);
	if(rNL)
		lo = min(lo, lNH / rNL);

	int hi = lZ ? 0 : INT_MIN;

	if(rPL)
		hi = max(hi, lPH / rPL);
	if(rNH)
		hi = max(hi, lNL / rNH);
	if(rPH)
		hi = max(hi, lNH / rPH);
	if(rNL)
		hi = max(hi, lPL / rNL);

	assert(lo <= hi);
	int len = hi - lo + 1;

	double *p = xcalloc(len, sizeof(double));
	double discarded = 0;

	for (int ri = 0; ri < r.len; ri++)
	{
		if(ri + r.low == 0)
			discarded += r.p[ri];
		else for (int li = 0; li < l.len; li++)
		{
			int res = (li + l.low)/(ri + r.low);

			assert(res >= lo);
			assert(res <= hi);

			p[res - lo] += l.p[li] * r.p[ri];
		}
	}

	// Restore axiom (1)
	if(discarded > 0)
	{
		for (int i = 0; i < len; i++)
			p[i] /= 1.0 - discarded;
	}

	return (struct Prob){ .low = lo, .len = len, .p = p };
}

CLEAN_BIOP(struct Prob, p_cdiv)

/* Internal implementation of p_mul. Inconsistent on being in-place or not. */
static struct Prob _p_mulk(struct Prob p, signed int x)
{
	if(x == 0)
		return p_constant(0);
	if(x == 1)
		return p;
	if(x == 2)
		return p_add(p, p);
	if(x < 0)
		return p_negs(_p_mulk(p, -x));

	struct Prob v = _p_mulk(p, x/2);
	v = ((v.p == p.p) ? p_add : p_adds)(v, v);

	if(x % 2)
	{
		struct Prob _v = v;
		v = p_add(v, p);
		p_free(_v);
	}

	return v;
}

struct Prob p_mulk(struct Prob p, signed int x)
{
	struct Prob _p = p_dup(p);
	struct Prob r = _p_mulk(_p, x);

	if(r.p != _p.p)
		p_free(_p);

	return r;
}

struct Prob p_mulks(struct Prob p, signed int x)
{
	struct Prob r = _p_mulk(p, x);

	if(r.p != p.p)
		p_free(p);

	return r;
}

struct Prob p_merge(struct Prob l, struct Prob r, double q)
{
	int low = min(l.low, r.low);
	int high = max(l.low + l.len, r.low + r.len) - 1;
	int len = high - low + 1;

	double *p = xcalloc(len, sizeof(double));

	for (int i = 0; i < l.len; i++)
		p[i + l.low - low] = l.p[i];
	for (int i = 0; i < r.len; i++)
		p[i + r.low - low] += r.p[i] * q;

	return (struct Prob){ .low = low, .len = len, .p = p };
}

struct Prob p_merges(struct Prob l, struct Prob r, double q)
{
	struct Prob res = p_merge(l, r, q);
	p_free(l);

	if(l.p != r.p)
		p_free(r);

	return res;
}

struct Prob p_muls(struct Prob l, struct Prob r)
{
	struct Prob sum = { };

	for (int i = 0; i < l.len; i++)
	{
		struct Prob cur = p_mulk(r, i + l.low);
		sum = p_merges(i ? sum : ((struct Prob){ .low = cur.low }), cur, l.p[i]);
	}

	p_free(l);

	if(l.p != r.p)
		p_free(r);

	return sum;
}

struct Prob p_selectsOne(struct Prob p, int of, bool selHigh)
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

void p_incr(struct Prob *p, struct Prob q)
{
	struct Prob r = p_add(*p, q);
	p_free(*p);
	*p = r;
}

struct Prob p_selects(struct Prob p, int sel, int of, bool selHigh, bool explode)
{
	// Use the MUCH faster selectOne algorithm (O(n) vs O(n!)
	if(sel == 1 && !explode)
		return p_selectsOne(p, of, selHigh);
	if(sel == of && !explode)
		return p_mulks(p, sel);

	// explosions must select high
	assert(!explode || selHigh);

	int *v = xcalloc(of, sizeof(int));
	int low = sel * p.low + (of/EXPLODE_RATIO)*(explode && p.low < 0 ? p.low : 0);
	int high = (sel + explode*of/EXPLODE_RATIO) * p_h(p);
	struct Prob c = (struct Prob){ .len = high - low + 1, .low = low };
	c.p = xcalloc(c.len, sizeof(double));

	struct Prob hitV = p_constant(0);
	int critC = 0;

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

		if(explode)
		{
			int crit = 0;

			for (int i = 0; i < of && v[i] == p.len - 1; i++)
				crit += i % EXPLODE_RATIO == EXPLODE_RATIO - 1;

			if(crit != critC)
			{
				assert(crit == critC + 1);
				p_incr(&hitV, p);
				critC = crit;
			}
		}

		for (int i = 0; i < hitV.len; i++)
			c.p[sum + hitV.low + i] += q * hitV.p[i];
	} while(combinations(p.len, of, v));

	free(v);
	p_free(hitV);
	p_free(p);

	return c;
}

struct Prob p_selects_bust(struct Prob p, int sel, int of, int bust, bool explode)
{
	assert(sel > 0 && sel <= of);
	assert(bust > 0 && bust <= of);
	assert(p.len > 0);

	const int bustV = p.low - 1;
	struct Prob total = p_constant(bustV);

	if(p.len == 1)
		return total;

	int *const choose = chooseBuf(of);
	const double p1 = *p.p;
	// p without 1s
	const struct Prob p2 = p_sans(p, SINGLETON(p.low, p.low));

	// range over # of 1s
	for (int n = 0; n < bust; n++)
	{
		// prob. of rolling n 1s
		double p_n = pow(p1, n) * pow(1 - p1, of - n) * (n ? choose[n-1] : 1);

		int left = of - n;

		// value distribution for that amount of 1s
		struct Prob vals = p_selects(p_dup(p2), min(sel, left), left, true, explode);

		// pad selection with 1s
		if(sel > left)
			vals.low += (sel - left) * vals.low;

		total = p_merges(total, vals, p_n);
		// keep track of covered cases
		total.p[bustV - total.low] -= p_n;
	}

	p_free(p2);
	free(choose);

	return total;
}

struct Prob p_cuts(struct Prob p, int l, int r)
{
	for (; p.p[l] <= 0.0; l++)
		;

	if(l == p.len)
	{
		p_free(p);
		return (struct Prob){ p.low, 0, NULL };
	}

	for (; p.p[p.len - r - 1] <= 0.0; r++)
		;

	p.len -= l + r;
	p.low += l;

	assert(p.len > 0);
	memmove(p.p, p.p + l, p.len * sizeof(*p.p));
	void *rp = realloc(p.p, sizeof(*p.p) * p.len);

	if(rp)
		p.p = rp;

	return p;
}

struct Prob p_explodes(struct Prob p)
{
	assert(p.len > 1);

	struct Prob exp = p_add(p, P_CONST(p_h(p)));
	struct Prob imp = p_adds(p_constant(p.low), p_negs(p_dup(p)));
	double Pmin = p.p[0];
	double Pmax = p.p[p.len - 1];

	// trim out min & max
	p.len -= 2;
	p.low++;
	memmove(p.p, p.p + 1, p.len * sizeof(double));
	p.p = xrealloc(p.p, p.len * sizeof(double));

	return p_merges(p_merges(p, exp, Pmax), imp, Pmin);
}

struct Prob p_bool(double prob)
{
	if(prob == 0)
		return p_constant(0);
	else if(prob == 1)
		return p_constant(1);
	else
	{
		struct Prob p = (struct Prob){ .len = 2, .low = 0, .p = xmalloc(2 * sizeof(double)) };
		p.p[1] = prob;
		p.p[0] = 1 - p.p[1];
		return p;
	}
}

/* P(x >= k) */
static double p_geqK(struct Prob x, int k)
{
	double pgt = 0;

	// start at equivalent index
	for (int j = max(0, k - x.low); j < x.len; j++)
		pgt += x.p[j];

	return pgt;
}

/* P(l <= r) */
double p_leq(struct Prob l, struct Prob r)
{
	double prob = 0.0;

	for (int i = 0; i < l.len; i++)
		prob += l.p[i] * p_geqK(r, l.low + i);

	return prob;
}

CLEAN_BIOP(double, p_leq)

double p_eq(struct Prob l, struct Prob r)
{
	double prob = 0.0;

	for (int i = 0; i < l.len; i++)
		prob += l.p[i] * probof(r, i + l.low);

	return prob;
}

CLEAN_BIOP(double, p_eq)

/* P(x > 0) */
static double p_true(struct Prob x)
{
	double prob = 0;

	for (int i = (x.low <= 0) ? -x.low+1 : 0; i < x.len; i++)
		prob += x.p[i];

	return prob;
}

struct Prob p_coalesces(struct Prob l, struct Prob r)
{
	if(l.low > 0)
	{
		p_free(r);
		return l;
	}

	int drop = -l.low + 1;

	if(drop >= l.len)
	{
		p_free(l);
		return r;
	}

	double p = p_true(l);
	return p_merges(p_cuts(l, drop, 0), r, 1 - p);
}

struct Prob p_scales(struct Prob p, double k)
{
	assert(k > 0);

	for (int i = 0; i < p.len; i++)
		p.p[i] *= k;

	return p;
}

struct Prob p_terns(struct Prob cond, struct Prob then, struct Prob otherwise)
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

struct Prob p_explode_ns(const struct Prob p, int n)
{
	assert(p.len > 1);
	assert(n > 0);

	int max = p.len + p.low - 1;
	double pMax = p.p[p.len - 1];

	// Axiom (1) gets restored over the loop
	struct Prob res = p_dup((struct Prob){ .len = p.len - 1, .low = p.low, .p = p.p });
	double pCur = pMax;

	for (int i = 1; i < n; i++, pCur *= pMax)
	{
		struct Prob cur = p_merge(res, (struct Prob){ .len = p.len - 1, .low = p.low + max * i, .p = p.p }, pCur);
		p_free(res);
		res = cur;
	}

	// final round without cutting off the maximum. Effectively cut off the converging infinite series, restoring Axiom (1)
	return p_merges(res, (struct Prob){ .len = p.len, .low = p.low + max * n, .p = p.p }, pCur);
}

struct Prob p_maxs(struct Prob l, struct Prob r)
{
	struct Prob res;
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
struct Prob p_mins(struct Prob l, struct Prob r)
{
	struct Prob res;
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

struct Prob p_dies(struct Prob p)
{
	assert(p.len >= 1);

	struct Prob sum = p_scales(p_uniform(p.low), p.p[0]);

	for (int i = 1; i < p.len; i++)
	{
		if(p.p[1] > 0)
			sum = p_merges(sum, p_uniform(p.low + i), p.p[1]);
	}

	p_free(p);
	return sum;
}

#undef CLEAN_BIOP