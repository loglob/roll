/* Implements translate() that generates a probability function from a die expression. */
#pragma once
#include "prob.h"
#include "parse.h"

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
			
		case '~':
			return p_rerolls(translate(d->reroll.v), d->reroll.count, d->reroll.ls);

		case '\\':
			return p_sans(translate(d->reroll.v), d->reroll.count, d->reroll.ls);

		case '!':
			return p_explodes(translate(d->unop));

		default:
			eprintf("Invalid die expression; Unknown operator '%c'\n", d->op);
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
			d_print(d->biop.l);
			printf(" %c ", d->op);
			d_print(d->biop.r);
		break;

		case '!':
			d_print(d->unop);
			putchar('!');
		break;
		
		case '^':
		case '_':
			d_print(d->select.v);
			printf(" ^%u/%u", d->select.sel, d->select.of);
		break;
			
		case '~':
		case '\\':
			d_print(d->select.v);
			printf(" %c", d->op);
			prls(d->reroll.ls, d->reroll.count);
		break;

		case '(':
			putchar('(');
			d_print(d->unop);
			putchar(')');
		break;

		default:
			eprintf("Invalid die expression; Unknown operator '%c'\n", d->op);
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

		b('+', "ADD")
		b('-', "SUB")
		b('x', "UNCACHED MUL")
		b('*', "CACHED MUL")
		b('/', "CACHED DIV")

		case '^':
			printf("SELECT %u HIGHEST FROM %u\n", d->select.sel, d->select.of);
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
			prls(d->reroll.ls, d->reroll.count);
			putchar('\n');
			d_printTree(d->select.v, depth + 1);
		break;

		#undef b
	}
}
