// sim.h: Implements a die roll simulator
#pragma once
#include "parse.h"
#include "settings.h"

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

/* Simulates a die roll.
	If v (verbose) is true outputs additional information for each roll, such as intermediate results. */
int sim(struct dieexpr *d)
{
	switch(d->op)
	{
		#define biop(c, op) case c: { \
			int l = sim(d->biop.l), r = sim(d->biop.r); \
			int res = l op r; \
			if(settings.verbose) printf("%d %c %d = %d\n", l, c, r, res); \
			return res; }

		biop('+', +)
		biop('-', -)
		biop('*', *)
		biop('/', /)

		#undef biop

		case '(':
			return sim(d->unop);

		case '^':
		case '_':
		{
			int *buf = xcalloc(sizeof(int), d->select.of);

			for (int i = 0; i < d->select.of; i++)
				buf[i] = sim(d->select.v);
			
			qsort(buf, d->select.of, sizeof(int), intpcomp);
			int *selbuf = buf + ((d->op == '_') ? 0 : (d->select.of - d->select.sel));
			int sum = sumls(selbuf, d->select.sel);
			
			if(settings.verbose)
			{
				printf("Got ");
				prls(buf, d->select.of);
				printf(" and selected ");
				prlsd(selbuf, d->select.sel, " + ");

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

			if(in(d->reroll.count, d->reroll.ls, r1))
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

			while(in(d->reroll.count, d->reroll.ls, r))
			{
				if(settings.verbose)
					printf("Discarded %d\n", r);
				
				r = sim(d->reroll.v);
			}

			return r;
		}

		case 'd':
		{
			int r = roll(d->constant);

			if(settings.verbose)
				printf("Rolled a %d on a d%u\n", r, d->constant);
	
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
		};

		case INT:
		{
			return d->constant;	
		};
		
		default:
			eprintf("Invalid die expression; Unknown operator '%c'\n", d->op);
	}
}