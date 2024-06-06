#include "die.h"
#include "parse.h"
#include "pattern.h"
#include "prob.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


CLONE(d_clone, struct Die)
FREE_P(d_free, struct Die)

void d_free(struct Die d)
{
	if(strchr(BIOPS, d.op))
	{
		d_freeP(d.biop.l);
		d_freeP(d.biop.r);
	}
	else if(strchr(SELECT, d.op))
		d_freeP(d.select.v);
	else if(strchr(REROLLS, d.op))
	{
		d_freeP(d.reroll.v);
		set_free(d.reroll.set);
	}
	else if(strchr(UOPS, d.op))
		d_freeP(d.unop);
	else if(d.op == '[')
	{
		d_freeP(d.match.v);

		for (int i = 0; i < d.match.cases; i++)
		{
			pt_free(d.match.patterns[i]);

			if(d.match.actions)
				d_free(d.match.actions[i]);
		}

		free(d.match.patterns);
		free(d.match.actions);
	}
}

bool d_boolean(struct Die *d)
{
	switch(d->op)
	{
		case '?':
			return d_boolean(d->biop.l) && d_boolean(d->biop.r);

		case ':':
			return d_boolean(d->ternary.then) && d_boolean(d->ternary.otherwise);

		default:
			return strchr(RELOPS, d->op);
	}
}

void d_print(struct Die *d)
{
	switch(d->op)
	{
		case INT:
			printf("%d", d->constant);
		break;

		case '@':
			putchar('@');
		break;

		case 'd':
			putchar('d');
			d_print(d->unop);
		break;

		case 'x':
			if(d->biop.l->op == INT && d->biop.r->op == 'd')
			{ // Special case for regular die strings
				printf("%u", d->biop.l->constant);
				d_print(d->biop.r);
				break;
			}
		// fallthrough
		case '-':
		case '*':
		case '+':
		case '/':
		case '?':
		case UPUP:
		case __:
			d_print(d->biop.l);
			printf(" %s ", tkstr(d->op));
			d_print(d->biop.r);
		break;

		case '!':
			d_print(d->unop);
			putchar('!');
		break;

		case '$':
			d_print(d->explode.v);
			putchar('$');
			printf("%d", d->explode.rounds);
		break;

		case '>':
		case '<':
		case LT_EQ:
		case GT_EQ:
		case '=':
		case NEQ:
			putchar('(');
			d_print(d->biop.l);
			printf(") %s (", tkstr(d->op));
			d_print(d->biop.r);
			putchar(')');
		break;

		case '^':
		case '_':
		case DOLLAR_UP:
			d_print(d->select.v);
			printf(" %s%u/%u", tkstr(d->op), d->select.sel, d->select.of);
		break;

		case UP_BANG:
		case UP_DOLLAR:
			d_print(d->select.v);
			printf(" %s%u/%u/%u", tkstr(d->op), d->select.sel, d->select.of, d->select.bust);
		break;

		case '~':
		case '\\':
			d_print(d->select.v);
			printf(" %c", d->op);
			set_print(d->reroll.set);
		break;

		case ':':
			putchar('(');
			d_print(d->ternary.cond);
			printf(" ? ");
			d_print(d->ternary.then);
			printf(" : ");
			d_print(d->ternary.otherwise);
			putchar(')');
		break;

		case '(':
			putchar('(');
			d_print(d->unop);
			putchar(')');
		break;

		case '[':
			d_print(d->match.v);
			printf("[ ");

			for (int i = 0; i < d->match.cases; i++)
			{
				if(i)
					printf("; ");

				pt_print(d->match.patterns[i]);

				if(d->match.actions)
				{
					printf(": ");
					d_print(d->match.actions + i);
				}
			}

			printf(" ]");
		break;

		default:
			eprintf("Invalid die expression; Unknown operator %s\n", tkstr(d->op));
	}
}

static void printIndent(int depth)
{
	for (int i = 0; i < depth; i++)
		printf("|   ");
}

void d_printTree(struct Die *d, int depth)
{
	printIndent(depth);

	switch(d->op)
	{
		#define b(c,m) case c: printf("%s\n", m); d_printTree(d->biop.l, depth + 1); d_printTree(d->biop.r, depth + 1); break;

		case INT:
			printf("%u\n", d->constant);
		break;

		case '@':
			printf("RETRIEVE MATCH CONTEXT\n");
		break;

		case 'd':
			if(d->unop->op == INT)
				printf("1d%u\n", d->unop->constant);
			else
			{
				printf("DYNAMICALLY SIZED DIE\n");
				d_printTree(d->unop, depth + 1);
			}
		break;

		case '(':
			printf("PARENTHESIZED\n");
			d_printTree(d->unop, depth + 1);
		break;

		case '[':
			printf("PATTERN %s\n", d->match.actions ? "MATCH" : "TEST");
			d_printTree(d->match.v, depth + 1);
			printIndent(depth);
			printf("AGAINST\n");
			for (int i = 0; i < d->match.cases; i++)
			{
				printIndent(depth);
				printf("  CASE ");
				pt_print(d->match.patterns[i]);

				if(d->match.actions)
				{
					printf(": \n");
					d_printTree(d->match.actions + i, depth + 1);
				}
				else
					putchar('\n');
			}
		break;

		case '!':
			printf("EXPLODING DICE\n");
			d_printTree(d->unop, depth + 1);
		break;

		case '$':
			printf("%u TIMES EXPLODING DICE\n", d->explode.rounds);
			d_printTree(d->explode.v, depth + 1);
		break;


		b('?', "ZERO COALESCENCE")

		b('>', "GREATER THAN")
		b('<', "LESS THAN")
		b(GT_EQ, "GREATER THAN OR EQUAL TO")
		b(LT_EQ, "LESS THAN OR EQUAL TO")
		b('=', "EQUAL TO")
		b(NEQ, "NOT EQUAL TO")

		b('+', "ADD")
		b('-', "SUB")
		b('x', "UNCACHED MUL")
		b('*', "CACHED MUL")
		b('/', "CACHED DIV")
		b(UPUP, "MAXIMUM")
		b(__, "MINIMUM")

		case ':':
			printf("TERNARY OPERATOR\n");
			d_printTree(d->ternary.cond, depth + 1);
			d_printTree(d->ternary.then, depth + 1);
			d_printTree(d->ternary.otherwise, depth + 1);
		break;

		case '^':
		case DOLLAR_UP:
			printf("SELECT %u HIGHEST FROM %u%s\n", d->select.sel, d->select.of,
				d->op == DOLLAR_UP ? " WITH EXPLOSIONS" : "");
			d_printTree(d->select.v, depth + 1);
		break;

		case UP_BANG:
		case UP_DOLLAR:
			printf("SELECT %u HIGHEST FROM %u WITH LESS THAN %u 1s%s\n", d->select.sel, d->select.of, d->select.bust,
				d->op == UP_DOLLAR ? " AND EXPLOSIONS" : "");
			d_printTree(d->select.v, depth + 1);
		break;

		case '_':
			printf("SELECT %u LOWEST FROM %u\n", d->select.sel, d->select.of);
			d_printTree(d->select.v, depth + 1);
		break;

		case '\\':
			printf("IGNORE ANY OF ");
		goto print_rerolls;

		case '~':
			printf("REROLL ANY OF ");
		print_rerolls:
			set_print(d->reroll.set);
			putchar('\n');
			d_printTree(d->select.v, depth + 1);
		break;

		default:
			fprintf(stderr, "WARN: Invalid die expression; Unknown operator %s\n", tkstr(d->op));

		#undef b
	}
}

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
			return p_rerolls(translate(ctx, d->reroll.v), d->reroll.set);

		case '\\':
			return p_sans(translate(ctx, d->reroll.v), d->reroll.set);

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
