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
			return p_cdiv(translate(d->biop.l), translate(d->biop.r));

		case '-':
			return p_adds(translate(d->biop.l), p_negs(translate(d->biop.r)));

		case '^':
		case '_':
			return p_select(translate(d->select.v), d->select.sel, d->select.of, d->op == '^');

		case UP_BANG:
		{
			struct prob p = translate(d->select.v);
			struct prob res = p_selectOne_bust(p, d->select.sel, d->select.of);
			p_free(p);

			return res;
		}

		case '~':
			return p_rerolls(translate(d->reroll.v), d->reroll.neg, d->reroll.set);

		case '\\':
			return p_sans(translate(d->reroll.v), d->reroll.neg, d->reroll.set);

		case '!':
			return p_explodes(translate(d->unop));

		case '$':
			return p_explode_ns(translate(d->explode.v), d->explode.rounds);

		case '<':
			return p_bool(1.0 - p_leq(translate(d->biop.r), translate(d->biop.l)));
		case '>':
			return p_bool(1.0 - p_leq(translate(d->biop.l), translate(d->biop.r)));
		case LT_EQ:
			return p_bool(p_leq(translate(d->biop.l), translate(d->biop.r)));
		case GT_EQ:
			return p_bool(p_leq(translate(d->biop.r), translate(d->biop.l)));
		case '=':
			return p_bool(p_eq(translate(d->biop.l), translate(d->biop.r)));

		case '?':
			return p_coalesces(translate(d->biop.l), translate(d->biop.r));

		case ':':
			return p_terns(translate(d->ternary.cond), translate(d->ternary.then), translate(d->ternary.otherwise));

		case UPUP:
			return p_maxs(translate(d->biop.l), translate(d->biop.r));

		case __:
			return p_mins(translate(d->biop.l), translate(d->biop.r));

		default:
			eprintf("Invalid die expression; Unknown operator %s\n", tkstr(d->op));
	}
}
