/* Implements translate() that generates a probability function from a die expression. */
#pragma once
#include "prob.h"
#include "die.h"
#include "set.h"
#include <stdio.h>

/* Transforms a dice expression to equivalent probability function. */
struct prob translate(struct die *d)
{
	switch(d->op)
	{
		case INT:
			return p_constant(d->constant);

		case 'd':
			return p_dies(translate(d->unop));

		case '(':
			return translate(d->unop);

		case 'x':
			return p_muls(translate(d->biop.l), translate(d->biop.r));

		case '*':
			return p_cmuls(translate(d->biop.l), translate(d->biop.r));

		case '+':
			return p_adds(translate(d->biop.l), translate(d->biop.r));

		case '/':
			return p_cdivs(translate(d->biop.l), translate(d->biop.r));

		case '-':
			return p_adds(translate(d->biop.l), p_negs(translate(d->biop.r)));

		case '^':
		case '_':
			return p_select(translate(d->select.v), d->select.sel, d->select.of, d->op == '^');

		case UP_BANG:
		{
			struct prob p = translate(d->select.v);
			struct prob res = p_select_bust(p, d->select.sel, d->select.of, d->select.bust);
			p_free(p);

			return res;
		}

		case '~':
			return p_rerolls(translate(d->reroll.v), d->reroll.set);

		case '\\':
			return p_sans(translate(d->reroll.v), d->reroll.set);

		case '!':
			return p_explodes(translate(d->unop));

		case '$':
			return p_explode_ns(translate(d->explode.v), d->explode.rounds);

		case '<':
			return p_bool(1.0 - p_leqs(translate(d->biop.r), translate(d->biop.l)));
		case '>':
			return p_bool(1.0 - p_leqs(translate(d->biop.l), translate(d->biop.r)));
		case LT_EQ:
			return p_bool(p_leqs(translate(d->biop.l), translate(d->biop.r)));
		case GT_EQ:
			return p_bool(p_leqs(translate(d->biop.r), translate(d->biop.l)));
		case '=':
			return p_bool(p_eqs(translate(d->biop.l), translate(d->biop.r)));
		case NEQ:
			return p_adds(p_constant(1), p_negs(p_bool(p_eq(translate(d->biop.l), translate(d->biop.r)))));

		case '?':
			return p_coalesces(translate(d->biop.l), translate(d->biop.r));

		case ':':
			return p_terns(translate(d->ternary.cond), translate(d->ternary.then), translate(d->ternary.otherwise));

		case UPUP:
			return p_maxs(translate(d->biop.l), translate(d->biop.r));

		case __:
			return p_mins(translate(d->biop.l), translate(d->biop.r));

		case '[':
		{
			struct prob running = translate(d->match.v);
			double total = 0.0;
			struct prob ret = {};

			for (int i = 0; i < d->match.cases; i++)
			{
				double hit = (1.0 - total) * pt_prob(d->match.patterns[i], &running);

				total += hit;

				if(d->match.actions && hit != 0.0)
				{
					struct prob tr = translate(d->match.actions + i);
					ret = i ? p_merges(ret, tr, hit) : p_scales(tr, hit);
				}
			}

			p_free(running);

			if(d->match.actions)
				return p_scales(ret, 1.0 / total);
			else
				return p_bool(total);
		}

		default:
			eprintf("Invalid die expression; Unknown operator %s\n", tkstr(d->op));
	}
}
