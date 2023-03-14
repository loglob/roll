// types.h: forward declaration for the basic types that make up die expression
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "xmalloc.h"
#include "util.h"

/** A range segment. Spans every integer between the given limits, inclusively.*/
struct range
{
	/** The lower limit */
	signed int start;
	/** The upper limit */
	signed int end;
};

/** A set of integers that permits ranges as primitives.
	Technically a Bag as there is no delete operation. */
struct set
{
	/** An array of rang segments, the disjunct union of which forms the overall set. Follows the axioms:
		(0) All segments are ordered such that start <= end
		(1) There are no overlapping ranges
		(2) The ranges are ordered low to high
		(3) Any two adjacent ranges are at least 1 apart, i.e. no two ranges could be merged
	*/
	struct range *entries;
	/** The amount of segments in the entries array. */
	size_t length;
	/** Whether the set is negated */
	bool negated;
};


/* represents a die expression as a syntax tree */
struct die
{
	char op;
	union
	{
		// valid if op == ':'
		struct { struct die *cond, *then, *otherwise; } ternary;
		// valid if op in BIOPS
		struct { struct die *l, *r; } biop;
		// valid if op in SELECT
		struct { struct die *v; int sel, of; } select;
		/** valid if op in REROLLS*/
		struct
		{
			/** The die to roll on */
			struct die *v;
			/** The results to reroll */
			struct set set;
		} reroll;
		// valid if op is '$'
		struct { struct die *v; int rounds; } explode;
		// valid if op is '['
		struct {
			struct die *v;
			/** The length of patterns and actions */
			int cases;
			/** The patterns that are matched against */
			struct pattern *patterns;
			/** The dice rolled when the corresponding pattern matches.
				May be NULL to indicate a pattern check rather than match. */
			struct die *actions;
		} match;
		// valid if op in UOPS
		struct die *unop;
		// valid if op == INT
		int constant;
	};
};

struct pattern
{
	/** The comparison operator, or 0 if this is a set pattern */
	char op;
	union
	{
		/** A set to match against. Valid if op is 0 */
		struct set set;
		/** A die to compare against, if op is in RELOPS */
		struct die die;
	};
};

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


CLONE(pt_clone, struct pattern)
CLONE(d_clone, struct die)

void set_free(struct set set);
void d_free(struct die die);
void pt_free(struct pattern pt);
void p_free(struct prob p);

FREE_P(d_free, struct die)
FREE_P(pt_free, struct pattern)

void d_print(struct die *d);
void pt_print(struct pattern p);
void set_print(struct set s);

bool pt_matches(struct pattern p, int x);
double pt_prob(struct pattern pt, struct prob *p);
