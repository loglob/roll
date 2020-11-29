#pragma once
#include "util.h"

/* Encodes the lowest/highest negative/positive values in a range.
	Values set to 0 if not within the range. */
typedef struct rlim {
	// the highest and lowest positive values
	signed int hpos, lpos;
	// the lowest and highest negative values
	signed int lneg, hneg;
	// the overall lowest and highest values
	signed int low, high;
	// the total length
	int len;
	// whether the range contains zero.
	bool haszero;
} rl_t;

// sets .low, .high and .len for a range with every other field set.
static rl_t _range_fin(rl_t s)
{
	s.low = min0(s.lpos, s.lneg);
	s.high = max(s.hpos, s.hneg);

	if(s.haszero)
	{
		s.low = min(s.low, 0);
		s.high = max(s.high, 0);
	}

	s.len = s.high - s.low + 1;

	return s;
}

/* Determines the limits of the given range. */
rl_t range_lim(signed int low, signed int high)
{
	if(low == 0) // Range starting at 0 that may be empty
		return (rl_t){ .lpos = (low != high), .hpos = high, .low = low, .high = high, .haszero = true, .len = high - low + 1 };
	else if(high == 0) // Range ending in 0
		return (rl_t){ .lneg = low, .hneg = -1, .low = low, .high = high, .haszero = true, .len = high - low + 1 };
	else if(low > 0) // all values are positive
		return (rl_t){ .lpos = low, .hpos = high, .low = low, .high = high, .len = high - low + 1 };
	else if(high < 0) // all values are negative
		return (rl_t){ .lneg = low, .hneg = high, .low = low, .high = high, .len = high - low + 1 };
	else // range crosses 0
		return (rl_t){ .lneg = low, .hneg = -1, .lpos = 1, .hpos = high, .low = low, .high = high, .haszero = true, .len = high - low + 1 };
}

/* Determines the range of results of (llow..lhigh)/(rlow..rhigh) */
rl_t range_div(signed int llow, signed int lhigh, signed int rlow, signed int rhigh)
{
	rl_t l = range_lim(llow, lhigh), r = range_lim(rlow, rhigh);
	#define DIV(a,b) b ? (a/b) : 0

	return _range_fin((rl_t){
		.lneg = min0(DIV(l.hpos, r.hneg), DIV(l.lneg, r.lpos)),
		.hneg = max(DIV(l.lpos, r.lneg), DIV(l.hneg, r.hpos)),
		.lpos = min0(DIV(l.lpos, r.hpos), DIV(l.hneg, r.lneg)),
		.hpos = max(DIV(l.hpos, r.lpos), DIV(l.lneg, r.hneg)),
		.haszero = l.haszero
	});

	#undef DIV
}

/* Determines the range of results of (llow..lhigh)*(rlow..rhigh) */
rl_t range_mul(signed int llow, signed int lhigh, signed int rlow, signed int rhigh)
{
	rl_t l = range_lim(llow, lhigh), r = range_lim(rlow, rhigh);

	return _range_fin((rl_t){
		.lneg = min0(l.lneg * r.hpos, l.hpos * r.lneg),
		.hneg = max(l.hneg * r.lpos, l.lpos * r.hneg),
		.lpos = min0(l.hneg * r.hneg, l.lpos * r.lpos),
		.hpos = min0(l.lneg * r.lneg, l.hpos * r.hpos),
		.haszero = l.haszero || r.haszero
	});
}
