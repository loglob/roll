#include "translate.h"
#include "parse.h"
#include "prob.h"
#include "util.h"

struct Prob translate(struct Prob *ctx, const struct Die *d)
{
	switch(d->op)
	{
		case INT:
			return p_constant(d->constant);

		case 'd':
			return p_dies(translate(ctx, d->unop));

		case '@':
		{
			if(! ctx)
				eprintf("Invalid die expression; '@' outside of match context\n");
		
			return p_dup(*ctx);
		}

		case '(':
			return translate(ctx, d->unop);

		case 'x':
			return p_muls(translate(ctx, d->biop.l), translate(ctx, d->biop.r));

		case '*':
			return p_cmuls(translate(ctx, d->biop.l), translate(ctx, d->biop.r));

		case '+':
			return p_adds(translate(ctx, d->biop.l), translate(ctx, d->biop.r));

		case '/':
			return p_cdivs(translate(ctx, d->biop.l), translate(ctx, d->biop.r));

		case '-':
			return p_adds(translate(ctx, d->biop.l), p_negs(translate(ctx, d->biop.r)));

		case '^':
		case '_':
		case DOLLAR_UP:
			return p_selects(translate(ctx, d->select.v), d->select.sel, d->select.of, d->op != '_', d->op == DOLLAR_UP);

		case UP_BANG:
		case UP_DOLLAR:
			return p_selects_bust(translate(ctx, d->select.v), d->select.sel, d->select.of, d->select.bust, d->op == UP_DOLLAR);

		case '~':
		{
			struct PatternProb pt = pt_translate(ctx, *d->reroll.pat);

			return p_rerolls(translate(ctx, d->reroll.v), pt);
		}

		case '\\':
		{
			struct PatternProb pt = pt_translate(ctx, *d->reroll.pat);

			return p_sans(translate(ctx, d->reroll.v), pt);
		}

		case '!':
			return p_explodes(translate(ctx, d->unop));

		case '$':
			return p_explode_ns(translate(ctx, d->explode.v), d->explode.rounds);

		case '<':
			return p_bool(1.0 - p_leqs(translate(ctx, d->biop.r), translate(ctx, d->biop.l)));
		case '>':
			return p_bool(1.0 - p_leqs(translate(ctx, d->biop.l), translate(ctx, d->biop.r)));
		case LT_EQ:
			return p_bool(p_leqs(translate(ctx, d->biop.l), translate(ctx, d->biop.r)));
		case GT_EQ:
			return p_bool(p_leqs(translate(ctx, d->biop.r), translate(ctx, d->biop.l)));
		case '=':
			return p_bool(p_eqs(translate(ctx, d->biop.l), translate(ctx, d->biop.r)));
		case NEQ:
			return p_adds(p_constant(1), p_negs(p_bool(p_eq(translate(ctx, d->biop.l), translate(ctx, d->biop.r)))));

		case '?':
			return p_coalesces(translate(ctx, d->biop.l), translate(ctx, d->biop.r));

		case ':':
			return p_terns(translate(ctx, d->ternary.cond), translate(ctx, d->ternary.then), translate(ctx, d->ternary.otherwise));

		case UPUP:
			return p_maxs(translate(ctx, d->biop.l), translate(ctx, d->biop.r));

		case __:
			return p_mins(translate(ctx, d->biop.l), translate(ctx, d->biop.r));

		case '[':
		{
			struct Prob running = translate(ctx, d->match.v);
			struct Prob result = {};

			for (int i = 0; i < d->match.cases; i++)
			{
				struct PatternProb pt = pt_translate(ctx, d->match.patterns[i]);
				struct Prob hit = pt_probs(pt, &running);
				pp_free(pt);
				
				double pHit = p_norms(&hit);

				if(d->match.actions && pHit > 0.0)
				{
					struct Prob action = translate(&hit, d->match.actions + i);
					result = p_merges(result, action, pHit);
				}
				else
					p_free(hit);
			}

			double pMiss = p_sum(running);
			p_free(running);

			if(d->match.actions)
			{
				if(pMiss == 1.0)
					eprintf("Invalid pattern match; All cases are impossible\n");

				return p_scales(result, 1.0 / (1.0 - pMiss));
			}
			else
				return p_bool(1.0 - pMiss);
		}

		default:
			eprintf("Invalid die expression; Unknown operator %s\n", tkstr(d->op));
	}
}

struct PatternProb pt_translate(struct Prob *ctx, struct Pattern p)
{
	struct PatternProb pp = { .op = p.op };

	if(p.op)
		pp.prob = translate(ctx, &p.die);
	else
		pp.set = p.set;
	
	return pp;
}
