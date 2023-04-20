// ranges.h: Implements operations on ranges
#pragma once
#include "util.h"
#include "set.h"
#include "die.h"
#include "settings.h"

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

// sets .low, .high and .len for a range with every other field set.
static rl_t _range_fin(rl_t s)
{
	s.low = min0(s.lPos, s.lNeg);
	s.high = max(s.hPos, s.hNeg);

	if(s.hasZero)
	{
		s.low = min(s.low, 0);
		s.high = max(s.high, 0);
	}

	s.len = s.high - s.low + 1;

	return s;
}

/** Determines the limits of the given range.
	@param low The overall lowest value
	@param low The overall highest value
	@return An equivalent range structure, with .low=low and .high=high
*/
rl_t range_lim(signed int low, signed int high)
{
	if(low == 0) // Range starting at 0 that may be empty
		return (rl_t){ .lPos = (low != high), .hPos = high, .low = low, .high = high, .hasZero = true, .len = high - low + 1 };
	else if(high == 0) // Range ending in 0
		return (rl_t){ .lNeg = low, .hNeg = -1, .low = low, .high = high, .hasZero = true, .len = high - low + 1 };
	else if(low > 0) // all values are positive
		return (rl_t){ .lPos = low, .hPos = high, .low = low, .high = high, .len = high - low + 1 };
	else if(high < 0) // all values are negative
		return (rl_t){ .lNeg = low, .hNeg = high, .low = low, .high = high, .len = high - low + 1 };
	else // range crosses 0
		return (rl_t){ .lNeg = low, .hNeg = -1, .lPos = 1, .hPos = high, .low = low, .high = high, .hasZero = true, .len = high - low + 1 };
}

/* Determines the range of results of (l.low..l.high)/(r.low..r.high) */
rl_t range_div(rl_t l, rl_t r)
{
	#define DIV(a,b) b ? (a/b) : 0

	return _range_fin((rl_t){
		.lNeg = min0(DIV(l.hPos, r.hNeg), DIV(l.lNeg, r.lPos)),
		.hNeg = max(DIV(l.lPos, r.lNeg), DIV(l.hNeg, r.hPos)),
		.lPos = min0(DIV(l.lPos, r.hPos), DIV(l.hNeg, r.lNeg)),
		.hPos = max(DIV(l.hPos, r.lPos), DIV(l.lNeg, r.hNeg)),
		.hasZero = l.hasZero
	});

	#undef DIV
}

/* Determines the range of results of (l.low..l.high)*(r.low..r.high) */
rl_t range_mul(rl_t l, rl_t r)
{
	return _range_fin((rl_t){
		.lNeg = min0(l.lNeg * r.hPos, l.hPos * r.lNeg),
		.hNeg = max(l.hNeg * r.lPos, l.lPos * r.hNeg),
		.lPos = min0(l.hNeg * r.hNeg, l.lPos * r.lPos),
		.hPos = min0(l.lNeg * r.lNeg, l.hPos * r.hPos),
		.hasZero = l.hasZero || r.hasZero
	});
}


/** Determines the value range of a die expression
	@param d a non-NULL die expression
	@returns The range of values d may take
*/
rl_t d_range(struct die *d)
{
	if(strchr(RELOPS, d->op))
		// (over)simplify all boolean expressions down to this, can't be bothered to do intersection checks
		return range_lim(0,1);
	else switch(d->op)
	{
		case INT:
			return range_lim(d->constant, d->constant);

		case 'd':
		{
			rl_t u = d_range(d->unop);
			return range_lim(min(1, u.low), max(-1, u.high));
		}

		case 'x':
		case '*':
			return range_mul(d_range(d->biop.l), d_range(d->biop.r));

		case '/':
			return range_div(d_range(d->biop.l), d_range(d->biop.r));

		case '+':
		{
			rl_t l = d_range(d->biop.l), r = d_range(d->biop.r);

			return range_lim(l.low + r.low, l.high + r.high);
		}

		case '-':
		{
			rl_t l = d_range(d->biop.l), r = d_range(d->biop.r);

			return range_lim(l.low - r.high, l.high - r.low);
		}

		case '(':
			return d_range(d->unop);

		case '!':
		{
			rl_t r = d_range(d->unop);

			return range_lim(r.low - r.high, r.high * 2);
		}

		case '$':
		{
			rl_t r = d_range(d->explode.v);
			return range_lim(r.low, r.high * (1 + d->explode.rounds));
		}

		case '~':
			return d_range(d->reroll.v);

		case '?':
		{
			rl_t l = d_range(d->biop.l);

			if(l.low > 0)
				return l;

			rl_t r = d_range(d->biop.r);

			if(l.high <= 0)
				return r;

			if(settings.verbose)
				fprintf(stderr, "[!] Exploding a '?'-expression may yield unexpected outcomes for probability functions with roots\n");

			return range_lim(min(1, r.low), max(l.high, r.high));
		}

		case ':':
		{
			rl_t cond = d_range(d->ternary.cond);

			if(cond.low > 0)
				return d_range(d->ternary.then);
			else if(cond.high <= 0)
				return d_range(d->ternary.otherwise);

			rl_t then = d_range(d->ternary.then);
			rl_t otherwise = d_range(d->ternary.otherwise);

			return range_lim(min(then.low, otherwise.low), max(then.high, otherwise.high));
		}

		case '\\':
		{
			rl_t r = d_range(d->reroll.v);

			while(set_has(d->reroll.set, r.low))
				r.low++;
			while(set_has(d->reroll.set, r.high))
				r.high--;

			return range_lim(r.low, r.high);
		}

		case '^':
		case '_':
		{
			rl_t r = d_range(d->select.v);

			return range_lim(r.low * d->select.sel, r.high * d->select.sel);
		}

		case UP_BANG:
		{
			rl_t r = d_range(d->select.v);

			return range_lim(r.low - 1, r.high * d->select.sel);
		}

		case UPUP:
		{
			rl_t l = d_range(d->biop.l), r = d_range(d->biop.l);
			return range_lim(max(l.low, r.low), max(l.high, r.high));
		}

		case __:
		{
			rl_t l = d_range(d->biop.l), r = d_range(d->biop.l);
			return range_lim(min(l.low, r.low), min(l.high, r.high));
		}

		case '[':
		{
			if(! d->match.actions)
				return range_lim(0,1);

			assert(d->match.cases);

			rl_t cur = d_range(d->match.actions);

			for (int i = 1; i < d->match.cases; i++)
			{
				rl_t x = d_range(d->match.actions + i);
				cur = range_lim(min(x.low, cur.low), max(x.high, cur.high));
			}

			return cur;
		}

		default:
			eprintf("Invalid die expression; Unknown operator %s\n", tkstr(d->op));
	}
}