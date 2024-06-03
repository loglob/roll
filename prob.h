/* represents probability functions that map N onto Q, with the sum of every value equaling 1.
	any function ending on 's' acts in-place or frees its arguments after use. */
#pragma once
#include <stdbool.h>
#include "pattern.h"
#include "set.h"

/* The highest value of p */
#define p_h(p) ((p).low + (p).len - 1)

/* Represents a probability function that follows 4 axioms:
	(0) p ⊂ ℚ⁺∪{0}
	(1) ∑p = 1
	(2) p[0] > 0
	(3) p[len - 1] > 0 */
struct prob
{
	// the lowest value in p
	signed int low;
	// the length of p
	int len;
	// the probability values
	double *p;
};

struct patternProb
{
	char op;
	union
	{
		struct set set;
		struct prob prob;
	};
};

/* Frees a probability function. */
void p_free(struct prob p);

void pp_free(struct patternProb pp);

/** The probability of a result in p
	@param p A probability function
	@param num A result
	@returns P(x from p = num), i.e. p(num)
*/
double probof(struct prob p, signed int num);


/** Partitions a probability function by a pattern.
Determines how likely a probability function will hit a pattern. (!) May return an empty probability function
	@param pt the pattern
	@param p the input probability distribution. Overwritten with the probability of misses, which does NOT fulfill axiom (1).
	@returns The distribution of hits. Does NOT fulfill axiom (1).
 */
struct prob pt_probs(struct patternProb pt, struct prob *p);

/** Determines the probability of a pattern capturing a value */
double pt_hit(struct patternProb pt, int v);

/* Creates a uniform distribution of the range 1..n (inclusive), or n..-1 if n < 0. */
struct prob p_uniform(int n);

/* Generates the probability function of y = P(-x). In-place. */
struct prob p_negs(struct prob p);

/* Clones the given probability function. Exits on malloc failure. */
struct prob p_dup(struct prob p);

/* Creates a probability function that maps the given value to 1. */
struct prob p_constant(int val);

/** Determines the probability that a result of p is in a set */
double p_has(struct prob p, struct set set);

/** Emulates rolling on p and rerolling once if the value is in the given signed set. In-place. */
struct prob p_rerolls(struct prob p, struct set set);

/** Like p_rerolls, with unlimited rerolls. */
struct prob p_sans(struct prob p, struct set set);

/** Emulates rolling on l and r, then adding the results. */
struct prob p_add(struct prob l, struct prob r);

struct prob p_adds(struct prob l, struct prob r);

/** Emulates rolling on l and r, then multiplying the results. */
struct prob p_cmul(struct prob l, struct prob r);

struct prob p_cmuls(struct prob l, struct prob r);

/** l/r. Division by 0 is discarded. */
struct prob p_cdiv(struct prob l, struct prob r);

struct prob p_cdivs(struct prob l, struct prob r);

/** Emulates rolling x times on p and adding the results. */
struct prob p_mulk(struct prob p, signed int x);

/** Like p_mul, in-place. */
struct prob p_mulks(struct prob p, signed int x);

/** adds l onto r*q. Leaves l and r unchanged */
struct prob p_merge(struct prob l, struct prob r, double q);

/** adds l onto r*q. In-place. */
struct prob p_merges(struct prob l, struct prob r, double q);

/** Rolls on l and sums that many rolls of r. */
struct prob p_muls(struct prob l, struct prob r);

/** Emulates rolling on p of times, and then selecting the highest/lowest value.
	In-place.
 */
struct prob p_selectsOne(struct prob p, int of, bool selHigh);

/** Emulates rolling on p of times, then adding the sel highest/lowest rolls. In-place. */
struct prob p_selects(struct prob p, int sel, int of, bool selHigh, bool explode);

/** Like p_select with selHigh=true, but 'goes bust' if majority of rolls are lowest. Leaves p intact.
	@param sel How many values to select
	@param of How many values to select from
	@param bust How many 1s cause the selection to go bust
*/
struct prob p_selects_bust(struct prob p, int sel, int of, int bust, bool explode);


/** Cuts l values from the left and r values from the right of the given probability array.
	Restores axioms (2) and (3), ignoring (1). */
struct prob p_cuts(struct prob p, int l, int r);

/** stack-allocated p_constant() */
#define P_CONST(x) (struct prob){ .low = x, .len = 1, .p = (double[]){ 1.0 } }

/** Simulates rolling on p, adding another roll to the maximum result and subtracting another roll from the minimum result. In-place. */
struct prob p_explodes(struct prob p);

/** Creates a probability distribution such that P(x=1) = prob, and P(x=0) = 1-prob  */
struct prob p_bool(double prob);

/** P(l <= r) */
double p_leq(struct prob l, struct prob r);

double p_leqs(struct prob l, struct prob r);

/** P(l = r) */
double p_eq(struct prob l, struct prob r);

double p_eqs(struct prob l, struct prob r);

/* Simulates rolling on l, then replacing any values <= 0 with a roll on r. In-place. */
struct prob p_coalesces(struct prob l, struct prob r);

/* Multiplies every probability of p by k. In-place.
	Ignores axiom (1).
 */
struct prob p_scales(struct prob p, double k);

/* Simulates rolling on cond, returning 'then' if the result is > 0 and 'otherwise' otherwise. In-place. */
struct prob p_terns(struct prob cond, struct prob then, struct prob otherwise);

/* Simulates n rounds of exploding-only rolls on p. In-place. */
struct prob p_explode_ns(const struct prob p, int n);

/* Emulates rolling on l and r, then selecting the higher value. In-place. */
struct prob p_maxs(struct prob l, struct prob r);

/* Emulates rolling on l and r, then selecting the lower value. In-place. */
struct prob p_mins(struct prob l, struct prob r);

/* Emulates rolling on p, then rolling a fair die with that many pips */
struct prob p_dies(struct prob p);

/** Scales a probability function to fulfill axiom (1).
	@returns The sum of probabilities in p.
		1 if axiom (1) was already fulfilled.
 */
double p_norms(struct prob *p);

double p_sum(struct prob p);
