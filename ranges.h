// ranges.h: Implements operations on ranges
#pragma once
#include "die.h"


/* Encodes the lowest/highest negative/positive values in a range.
	Values set to 0 if not within the range. */
typedef struct rlim {
	// the highest and lowest positive values
	signed int hPos, lPos;
	// the lowest and highest negative values
	signed int lNeg, hNeg;
	// the overall lowest and highest values
	signed int low, high;
	// the total length
	int len;
	// whether the range contains zero.
	bool hasZero;
} rl_t;


/** Determines the limits of the given range.
	@param low The overall lowest value
	@param low The overall highest value
	@return An equivalent range structure, with .low=low and .high=high
*/
rl_t range_lim(signed int low, signed int high);

/* Determines the range of results of (l.low..l.high)/(r.low..r.high) */
rl_t range_div(rl_t l, rl_t r);

/* Determines the range of results of (l.low..l.high)*(r.low..r.high) */
rl_t range_mul(rl_t l, rl_t r);

/** Determines the value range of a die expression
	@param d a non-NULL die expression
	@returns The range of values d may take
*/
rl_t d_range(struct die *d);
