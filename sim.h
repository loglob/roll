// sim.h: Implements a die roll simulator
#pragma once
#include "die.h"
#include "ranges.h"
#include "settings.h"
#include "util.h"

/* Generates a random number in [1..pips] (uniform) */
static inline int roll(int pips)
{
	int largestMult = RAND_MAX - (RAND_MAX % pips);
	int rint;

	do
		rint = rand();
	while(rint > largestMult);

	return (rint % pips) + 1;
}

/** Simulates a die roll.
	If settings.verbose is true outputs additional information for each roll, such as intermediate results.
	@param p A die expression
	@returns The result of rolling d
*/
int sim(struct die *d)
{
	switch(d->op)
	{
		#define _biop(c, calc) case c: { \
			int l = sim(d->biop.l), r = sim(d->biop.r); \
			int res = calc; \
			if(settings.verbose) printf("%d %s %d = %d\n", l, tkstr(c), r, res); \
			return res; }

		#define biop(c, op) _biop(c, l op r)

		biop('<', <)
		biop('>', >)
		biop(LT_EQ, <=)
		biop(GT_EQ, >=)
		biop('=', ==)
		biop('+', +)
		biop('-', -)
		biop('*', *)
		biop('/', /)

		_biop(UPUP, max(l,r));
		_biop(__, min(l,r));

		#undef biop
		#undef _biop

		case '?':
		{
			int r1 = sim(d->biop.l);

			if(r1 <= 0)
			{
				int r2 = sim(d->biop.r);

				if(settings.verbose)
					printf("Rolled %d after coalescing %d\n", r2, r1);

				return r2;
			}

			return r1;
		}

		case ':':
		{
			int r1 = sim(d->ternary.cond);

			if(r1 > 0)
			{
				int r2 = sim(d->ternary.then);

				if(settings.verbose)
					printf("Rolled %d for ternary condition resulting in %d from true branch\n", r1, r2);

				return r2;
			}
			else
			{
				int r2 = sim(d->ternary.otherwise);

				if(settings.verbose)
					printf("Rolled %d for ternary condition resulting in %d from false branch\n", r1, r2);

				return r2;
			}

		}

		case '(':
			return sim(d->unop);

		case '^':
		case '_':
		case UP_BANG:
		{
			int *buf = xcalloc(sizeof(int), d->select.of);

			for (int i = 0; i < d->select.of; i++)
				buf[i] = sim(d->select.v);

			qsort(buf, d->select.of, sizeof(int), intpcomp);
			int *selected = buf + ((d->op == '_') ? 0 : (d->select.of - d->select.sel));
			int sum = sumls(selected, d->select.sel);

			if(d->op == UP_BANG)
			{
				rl_t lim = d_range(d->select.v);

				for (int i = 0; i <= d->select.of/2; i++)
				{
					if(buf[i] != lim.low)
						goto not_bust;
				}

				if(settings.verbose)
				{
					printf("Got ");
					prls(buf, d->select.of);
					printf(" and went bust\n");
				}

				sum = lim.low - 1;
			}
			else not_bust: if(settings.verbose)
			{
				printf("Got ");
				prls(buf, d->select.of);
				printf(" and selected ");
				prlsd(selected, d->select.sel, " + ");

				if(d->select.sel == 1)
					putchar('\n');
				else
					printf(" = %d\n", sum);
			}

			free(buf);
			return sum;
		}

		case '~':
		{
			int r1 = sim(d->reroll.v);

			if(set_has(d->reroll.set, r1))
			{
				int r2 = sim(d->reroll.v);

				if(settings.verbose)
					printf("Rolled %d after discarding %d\n", r2, r1);

				return r2;
			}

			return r1;
		}

		case '\\':
		{
			int r = sim(d->reroll.v);

			while(set_has(d->reroll.set, r))
			{
				if(settings.verbose)
					printf("Discarded %d\n", r);

				r = sim(d->reroll.v);
			}

			return r;
		}

		case 'd':
		{
			int pips = sim(d->unop);
			int r = roll(pips);

			if(settings.verbose)
				printf("Rolled a %d on a d%u\n", r, pips);

			return r;
		}

		case 'x':
		{
			int rolls = sim(d->biop.l);
			int *buf = xcalloc(sizeof(int), rolls);

			for (int i = 0; i < rolls; i++)
				buf[i] = sim(d->biop.r);

			int sum = sumls(buf, rolls);

			if(settings.verbose && rolls > 1)
			{
				//printf("Rolled ");
				prlsd(buf, rolls, " + ");
				printf(" = %d\n", sum);
			}

			free(buf);
			return sum;
		}

		case INT:
		{
			return d->constant;
		}

		case '$':
		{
			rl_t r = d_range(d->explode.v);
			int cur = sim(d->explode.v);
			int sum = cur;
			int i;

			for (i = 0; i < d->explode.rounds && cur == r.high; i++)
				sum += cur = sim(d->explode.v);

			if(i > 0 && settings.verbose)
				printf("Rolled a %d which exploded %d times to %d\n", r.high, i, sum);

			return sum;
		}

		case '!':
		{
			rl_t r = d_range(d->unop);
			int r1 = sim(d->unop);

			if(r1 == r.high)
			{
				int r2 = sim(d->unop);

				if(settings.verbose)
					printf("Rolled a %d which exploded to %d\n", r1, r1 + r2);

				return r1 + r2;
			}
			else if(r1 == r.low)
			{
				int r2 = sim(d->unop);

				if(settings.verbose)
					printf("Rolled a %d which imploded to %d\n", r1, r1 - r2);

				return r1 - r2;
			}
			else
				return r1;
		}

		default:
			eprintf("Invalid die expression; Unknown operator %s\n", tkstr(d->op));
	}
}