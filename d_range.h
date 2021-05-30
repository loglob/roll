#pragma once
#include "ranges.h"
#include "parse.h"

rl_t d_range(d_t *d)
{
	switch(d->op)
	{
		case INT:
			return range_lim(d->constant, d->constant);

		case 'd':
			return range_lim(1, d->constant);

		case 'x':
		case '*':
		{
			rl_t l = d_range(d->biop.l), r = d_range(d->biop.r);

			return range_mul(l.low, l.high, r.low, r.high);
		}

		case '/':
		{
			rl_t l = d_range(d->biop.l), r = d_range(d->biop.r);

			return range_div(l.low, l.high, r.low, r.high);
		}

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

		case '~':
		{
			return d_range(d->reroll.v);
		}

		case '\\':
		{
			rl_t r = d_range(d->reroll.v);

			while(in(d->reroll.count, d->reroll.ls, r.low))
				r.low++;
			while(in(d->reroll.count, d->reroll.ls, r.high))
				r.high--;
			
			return range_lim(r.low, r.high);
		}

		case '^':
		case '_':
		{
			rl_t r = d_range(d->select.v);

			return range_lim(r.low * d->select.sel, r.high * d->select.sel);
		}

		default:
			eprintf("Invalid die expression; Unknown operator '%c'\n", d->op);
	}
}