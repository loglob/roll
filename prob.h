/* represents probability functions that map N onto Q, with the sum of every value equaling 1.
	any function ending on 's' acts in-place or frees its arguments after use. */
#pragma once
#include "ast.h"
#include "set.h"
#include <stdbool.h>


/** The highest value of p */
#define p_h(p) ((p).low + (p).len - 1)

/* Represents a probability function that follows 4 axioms:
	(0) p ⊂ ℚ⁺∪{0}
	(1) ∑p = 1
	(2) p[0] > 0
	(3) p[len - 1] > 0
 */
struct Prob
{
	/** the lowest value in p */
	signed int low;
	/** the length of p  */
	int len;
	/** the probability values */
	double *p;
};

/** Represents the probability distribution of a pattern */
struct PatternProb
{
	/** Either 0, or a relational operator */
	char op;
	union
	{
		/** Valid if op is 0 */
		struct SetPattern set;
		/** Valid if op isn't 0 */
		struct Prob prob;
	};
};

/** Frees a probability function. */
void p_free(struct Prob p);

void pp_free(struct PatternProb pp);

/** The probability of a result in p
	@param p A probability function
	@param num A result
	@returns P(x from p = num), i.e. p(num)
*/
double probof(struct Prob p, signed int num) PURE_ATTR;

/** Partitions a probability function by a pattern.
	Determines how likely a probability function will hit a pattern. (!) May return an empty probability function
	@param pt the pattern
	@param p the input probability distribution. Overwritten with the probability of misses, which does NOT fulfill axiom (1).
	@returns The distribution of hits. Does NOT fulfill axiom (1).
 */
struct Prob pt_probs(struct PatternProb pt, struct Prob *p);

/** Determines the probability of a pattern capturing a value
	@param p The probability function that `v` was picked from
 */
double pt_hit(struct PatternProb pt, const struct Prob *p, int v) PURE_ATTR;

/** Creates a uniform distribution of the range 1..n (inclusive), or n..-1 if n < 0. */
struct Prob p_uniform(int n) PURE_ATTR;

/** Generates the probability function of y = P(-x). In-place. */
struct Prob p_negs(struct Prob p);

/** Clones the given probability function. Exits on malloc failure. */
struct Prob p_dup(struct Prob p);

/** Creates a probability function that maps the given value to 1. */
struct Prob p_constant(int val);

/** Determines the probability that a result of p is in a set */
double p_has(struct Prob p, struct Set set) PURE_ATTR;

/** Emulates rolling on p and rerolling once if the value is in the given signed set. In-place. */
struct Prob p_rerolls(struct Prob p, struct PatternProb pat);

/** Like p_rerolls, with unlimited rerolls. */
struct Prob p_sans(struct Prob p, struct PatternProb pat);

/** Emulates rolling on l and r, then adding the results. */
struct Prob p_add(struct Prob l, struct Prob r);

struct Prob p_adds(struct Prob l, struct Prob r);

/** Emulates rolling on l and r, then multiplying the results. */
struct Prob p_cmul(struct Prob l, struct Prob r);

struct Prob p_cmuls(struct Prob l, struct Prob r);

/** l/r. Division by 0 is discarded. */
struct Prob p_cdiv(struct Prob l, struct Prob r);

struct Prob p_cdivs(struct Prob l, struct Prob r);

/** Emulates rolling x times on p and adding the results. */
struct Prob p_mulk(struct Prob p, signed int x);

/** Like p_mul, in-place. */
struct Prob p_mulks(struct Prob p, signed int x);

/** adds l onto r*q. Leaves l and r unchanged */
struct Prob p_merge(struct Prob l, struct Prob r, double q);

/** adds l onto r*q. In-place. */
struct Prob p_merges(struct Prob l, struct Prob r, double q);

/** Rolls on l and sums that many rolls of r. */
struct Prob p_muls(struct Prob l, struct Prob r);

/** Emulates rolling on p of times, and then selecting the highest/lowest value.
	In-place.
 */
struct Prob p_selectsOne(struct Prob p, int of, bool selHigh);

/** Emulates rolling on p of times, then adding the sel highest/lowest rolls. In-place. */
struct Prob p_selects(struct Prob p, int sel, int of, bool selHigh, bool explode);

/** Like p_select with selHigh=true, but 'goes bust' if majority of rolls are lowest. Leaves p intact.
	@param sel How many values to select
	@param of How many values to select from
	@param bust How many 1s cause the selection to go bust
*/
struct Prob p_selects_bust(struct Prob p, int sel, int of, int bust, bool explode);


/** Cuts l values from the left and r values from the right of the given probability array.
	Restores axioms (2) and (3), ignoring (1).
 */
struct Prob p_cuts(struct Prob p, int l, int r);

/** stack-allocated p_constant() */
#define P_CONST(x) (struct Prob){ .low = x, .len = 1, .p = (double[]){ 1.0 } }

/** Simulates rolling on p, adding another roll to the maximum result and subtracting another roll from the minimum result. In-place. */
struct Prob p_explodes(struct Prob p);

/** Creates a probability distribution such that P(x=1) = prob, and P(x=0) = 1-prob  */
struct Prob p_bool(double prob);

/** P(l <= r) */
double p_leq(struct Prob l, struct Prob r);

double p_leqs(struct Prob l, struct Prob r);

/** P(l = r) */
double p_eq(struct Prob l, struct Prob r);

double p_eqs(struct Prob l, struct Prob r);

/** Simulates rolling on l, then replacing any values <= 0 with a roll on r. In-place. */
struct Prob p_coalesces(struct Prob l, struct Prob r);

/** Multiplies every probability of p by k. In-place.
	Ignores axiom (1).
 */
struct Prob p_scales(struct Prob p, double k);

/** Simulates rolling on cond, returning 'then' if the result is > 0 and 'otherwise' otherwise. In-place. */
struct Prob p_terns(struct Prob cond, struct Prob then, struct Prob otherwise);

/** Simulates n rounds of exploding-only rolls on p. In-place. */
struct Prob p_explode_ns(const struct Prob p, int n);

/** Emulates rolling on l and r, then selecting the higher value. In-place. */
struct Prob p_maxs(struct Prob l, struct Prob r);

/** Emulates rolling on l and r, then selecting the lower value. In-place. */
struct Prob p_mins(struct Prob l, struct Prob r);

/** Emulates rolling on p, then rolling a fair die with that many pips */
struct Prob p_dies(struct Prob p);

/** Scales a probability function to fulfill axiom (1).
	@returns The sum of probabilities in p.
		1 if axiom (1) was already fulfilled.
 */
double p_norms(struct Prob *p);

double p_sum(struct Prob p);

struct Prob p_udivs(struct Prob l, struct Prob r);
