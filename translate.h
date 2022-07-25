/* Implements translate() that generates a probability function from a die expression. */
#pragma once
#include "prob.h"
#include "parse.h"
#include "set.h"
#include <stdio.h>

/* Transforms a dice expression to equivalent probability function. */
p_t translate(d_t *d)
{
	switch(d->op)
	{
		case INT:
			return p_constant(d->constant);

		case 'd':
			return p_uniform(d->constant);

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
			assert(d->select.sel == 1);

			struct prob res = p_selectOne_bust(p, d->select.of);
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
			return p_bool(p_lt(translate(d->biop.l), translate(d->biop.r)));
		case '>':
			return p_bool(p_lt(translate(d->biop.r), translate(d->biop.l)));

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

/* Prints the given die expression */
void d_print(d_t *d)
{
	switch(d->op)
	{
		case INT:
			printf("%d", d->constant);
		break;

		case 'd':
			printf("1d%u", d->constant);
		break;

		case 'x':
			if(d->biop.l->op == INT && d->biop.r->op == 'd')
			{ // Special case for regular die strings
				printf("%ud%u", d->biop.l->constant, d->biop.r->constant);
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
			putchar('(');
			d_print(d->biop.l);
			printf(") %c (", d->op);
			d_print(d->biop.r);
			putchar(')');
		break;

		case '^':
		case '_':
			d_print(d->select.v);
			printf(" ^%u/%u", d->select.sel, d->select.of);
		break;

		case UP_BANG:
			d_print(d->select.v);
			printf(" ^!%u", d->select.of);
		break;

		case '~':
		case '\\':
			d_print(d->select.v);
			printf(" %c", d->op);
			if(d->reroll.neg)
				putchar('!');
			set_print(d->reroll.set, stdout);
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

		default:
			eprintf("Invalid die expression; Unknown operator %s\n", tkstr(d->op));
	}

}

/* Prints the syntax tree of the given die expression */
void d_printTree(struct dieexpr *d, int depth)
{
	for (int i = 0; i < depth; i++)
		printf("|   ");

	switch(d->op)
	{
		#define b(c,m) case c: printf("%s\n", m); d_printTree(d->biop.l, depth + 1); d_printTree(d->biop.r, depth + 1); break;

		case INT:
			printf("%u\n", d->constant);
		break;

		case 'd':
			printf("1d%u\n", d->constant);
		break;

		case '(':
			printf("PARENTHESIZED\n");
			d_printTree(d->unop, depth + 1);
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
			printf("SELECT %u HIGHEST FROM %u\n", d->select.sel, d->select.of);
			d_printTree(d->select.v, depth + 1);
		break;

		case UP_BANG:
			printf("SELECT HIGHEST FROM %u WITHOUT GOING BUST\n", d->select.of);
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
			printf("REROLL ANY %s ", d->reroll.neg ? "BUT" : "OF");
		print_rerolls:
			set_print(d->reroll.set, stdout);
			putchar('\n');
			d_printTree(d->select.v, depth + 1);
		break;

		default:
			fprintf(stderr, "WARN: Invalid die expression; Unknown operator %s\n", tkstr(d->op));

		#undef b
	}
}
