#pragma once
#include "ranges.h"
#include "parse.h"
#include "settings.h"

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

		case '$':
		{
			rl_t r = d_range(d->explode.v);
			return range_lim(r.low, r.high * (1 + d->explode.rounds));
		}

		case '~':
		{
			return d_range(d->reroll.v);
		}

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

		case UPUP:
		{
			rl_t l = d_range(d->biop.l), r = d_range(d->biop.l);
			return range_lim(max(l.low, r.low), max(l.low, r.low));
		}

		default:
			eprintf("Invalid die expression; Unknown operator %s\n", tkstr(d->op));
	}
}