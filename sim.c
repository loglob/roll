#include "translate.h"
#include "parse.h"
#include "prob.h"
#include "settings.h"
#include "sim.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>


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

/** Checks whether a pattern matches an integer. May evaluate dice.
	@param p a pattern
	@param d the die that produced `x`
	@param x a value
	@param pBuf buffer that caches translation of `d`
	@return whether x is matched by p
 */
static bool pt_matches(const int *ctx, struct Pattern p, const struct Die *d, int x, struct Prob *pBuf)
{
	if(p.op)
	{
		struct Die c = (struct Die){ .op = INT, .constant = x };
		struct Die d = (struct Die){ .op = p.op, .biop = { .l = &c, .r = &p.die } };

		return sim(ctx, &d);
	}
	else
	{
		bool hit = set_has(p.set.entries, x);

		if(!hit && (p.set.hasMin || p.set.hasMax))
		{
			if(!pBuf->p)
			{
				struct Prob c = P_CONST(ctx ? *ctx : 0);
				*pBuf = translate(ctx ? &c : NULL, d);
			}

			hit = (p.set.hasMin && pBuf->low == x) || (p.set.hasMax && p_h(*pBuf) == x);
		}

		return p.set.negated ? !hit : hit;
	}
}

struct Range d_limits(const int *ctx, const struct Die *d)
{
	struct Prob c = P_CONST(ctx ? *ctx : -1);
	struct Prob p = translate(ctx ? &c : NULL, d);
	p_free(p);

	return (struct Range) {
		p.low, p_h(p)
	};
}

int sim(const int *ctx, struct Die *d)
{
	switch(d->op)
	{
		#define _biop(c, calc) case c: { \
			int l = sim(ctx, d->biop.l), r = sim(ctx, d->biop.r); \
			int res = calc; \
			if(settings.verbose) printf("%d %s %d = %d\n", l, tkstr(c), r, res); \
			return res; }

		#define biop(c, op) _biop(c, l op r)

		biop('<', <)
		biop('>', >)
		biop(LT_EQ, <=)
		biop(GT_EQ, >=)
		biop('=', ==)
		biop(NEQ, !=)
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
			int r1 = sim(ctx, d->biop.l);

			if(r1 <= 0)
			{
				int r2 = sim(ctx, d->biop.r);

				if(settings.verbose)
					printf("Rolled %d after coalescing %d\n", r2, r1);

				return r2;
			}

			return r1;
		}

		case ':':
		{
			int r1 = sim(ctx, d->ternary.cond);

			if(r1 > 0)
			{
				int r2 = sim(ctx, d->ternary.then);

				if(settings.verbose)
					printf("Rolled %d for ternary condition resulting in %d from true branch\n", r1, r2);

				return r2;
			}
			else
			{
				int r2 = sim(ctx, d->ternary.otherwise);

				if(settings.verbose)
					printf("Rolled %d for ternary condition resulting in %d from false branch\n", r1, r2);

				return r2;
			}

		}

		case '(':
			return sim(ctx, d->unop);

		case '^':
		case '_':
		case UP_BANG:
		case UP_DOLLAR:
		case DOLLAR_UP:
		{
			int *buf = xcalloc(d->select.of, sizeof(int));

			for (int i = 0; i < d->select.of; i++)
				buf[i] = sim(ctx, d->select.v);

			qsort(buf, d->select.of, sizeof(int), intpcomp);
			int *selected = buf + ((d->op == '_') ? 0 : (d->select.of - d->select.sel));
			int sum = sumls(selected, d->select.sel);

			// the minimum and maximum possible values of `d->select.v`
			struct Range vLimits;

			if(d->op == UP_BANG || d->op == UP_DOLLAR || d->op == DOLLAR_UP)
				vLimits = d_limits(ctx, d->select.v);

			if(d->op == UP_BANG || d->op == UP_DOLLAR)
			{	
				for (int i = 0; i < d->select.bust; i++)
				{
					if(buf[i] != vLimits.start)
						goto not_bust;
				}

				if(settings.verbose)
				{
					printf("Got ");
					prls(buf, d->select.of);
					printf(" and went bust\n");
				}

				sum = vLimits.start - 1;
				goto bust;
			}

			not_bust:
			if(settings.verbose)
			{
				printf("Got ");
				prls(buf, d->select.of);
				printf(" and selected ");
				sum = prSum(selected, d->select.sel);
			}
			else
				sum = sumls(selected, d->select.sel);

			if(d->op == UP_DOLLAR || d->op == DOLLAR_UP)
			{
				int n_max = 0;

				for (int i = d->select.of; --i && buf[i] == vLimits.end;)
					n_max++;

				int explosions = n_max/EXPLODE_RATIO;

				if(explosions <= 0)
					goto bust;

				int *exploded = xcalloc(explosions, sizeof(int));

				for (int i = 0; i < explosions; i++)
					exploded[i] = sim(ctx, d->select.v);

				if(settings.verbose)
				{
					printf("Exploded %u times, adding ", explosions);
					sum += prSum(exploded, explosions);
				}
				else
					sum += sumls(exploded, explosions);

				free(exploded);
			}

			bust:
			free(buf);
			return sum;
		}

		case '~':
		{
			int r1 = sim(ctx, d->reroll.v);
			struct Prob buf = {};
			bool match = pt_matches(ctx, *d->reroll.pat, d->reroll.v, r1, &buf);
			p_free(buf);

			if(match)
			{
				int r2 = sim(ctx, d->reroll.v);

				if(settings.verbose)
					printf("Rolled %d after discarding %d\n", r2, r1);



				return r2;
			}

			return r1;
		}

		case '\\':
		{
			int r = sim(ctx, d->reroll.v);
			struct Prob buf = {};

			while(pt_matches(ctx, *d->reroll.pat, d->reroll.v, r, &buf))
			{
				if(settings.verbose)
					printf("Discarded %d\n", r);

				r = sim(ctx, d->reroll.v);
			}

			p_free(buf);
			return r;
		}

		case 'd':
		{
			int pips = sim(ctx, d->unop);
			int r = roll(pips);

			if(settings.verbose)
				printf("Rolled a %d on a d%u\n", r, pips);

			return r;
		}

		case 'x':
		{
			int rolls = sim(ctx, d->biop.l);
			int *buf = xcalloc(rolls, sizeof(int));

			for (int i = 0; i < rolls; i++)
				buf[i] = sim(ctx, d->biop.r);

			int sum = sumls(buf, rolls);

			if(settings.verbose && rolls > 1)
				prSum(buf, rolls);

			free(buf);
			return sum;
		}

		case INT:
		{
			return d->constant;
		}

		case '@':
		{
			if(! ctx)
				eprintf("Invalid die expression; '@' outside match context");
			if(settings.verbose)
				printf("Retrieved from stack: %d\n", *ctx);
	
			return *ctx;
		}

		case '$':
		{
			struct Range lim = d_limits(ctx, d->explode.v);
			int cur = sim(ctx, d->explode.v);
			int sum = cur;
			int i;

			for (i = 0; i < d->explode.rounds && cur == lim.end; i++)
				sum += cur = sim(ctx, d->explode.v);

			if(i > 0 && settings.verbose)
				printf("Rolled a %d which exploded %d times to %d\n", lim.end, i, sum);

			return sum;
		}

		case '!':
		{
			struct Range lim = d_limits(ctx, d->unop);
			int r1 = sim(ctx, d->unop);

			if(r1 == lim.end)
			{
				int r2 = sim(ctx, d->unop);

				if(settings.verbose)
					printf("Rolled a %d which exploded to %d\n", r1, r1 + r2);

				return r1 + r2;
			}
			else if(r1 == lim.start)
			{
				int r2 = sim(ctx, d->unop);

				if(settings.verbose)
					printf("Rolled a %d which imploded to %d\n", r1, r1 - r2);

				return r1 - r2;
			}
			else
				return r1;
		}

		case '[':
		{
			struct Prob buf = {};

			while(true)
			{
				int r = sim(ctx, d->match.v);

				for (int i = 0; i < d->match.cases; i++)
				{
					if(pt_matches(ctx, d->match.patterns[i], d->match.v, r, &buf))
					{
						if(settings.verbose)
						{
							printf("Matched with case #%u: ", i);
							pt_print(d->match.patterns[i]);
							putchar('\n');
						}

						p_free(buf);

						if(d->match.actions)
							return sim(&r, d->match.actions + i);
						else
							return true;
					}
				}

				if(! d->match.actions)
				{
					p_free(buf);
					return false;
				}
			}
		}

		default:
			eprintf("Invalid die expression; Unknown operator %s\n", tkstr(d->op));
	}
}