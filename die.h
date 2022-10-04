// die.h: Defines the die struct and type.
#pragma once
#include <string.h>
#include "types.h"

#define NUL ((char)0)
#define INT ((char)-2)
#define ZERO ((char)-3)
// equivalent to (char)-4
#define UPUP '\xFC'
// equivalent to (char)-5
#define __ '\xFB'
// equivalent to (char)-6
#define UP_BANG '\xFA'
// equivalent to (char)-7
#define LT_EQ '\xF9'
// equivalent to (char)-8
#define GT_EQ '\xF8'
#define RELOPS "<>=\xF9\xF8"
#define BIOPS "+-*x/?" RELOPS "\xFC\xFB"
#define SELECT "^_\xFA"
#define REROLLS "~\\"
#define UOPS SELECT REROLLS "!$d("
#define SPECIAL BIOPS UOPS ",/():;[]"

extern const char *tkstr(char tok);

/** Frees all resources used by a die expression. */
void d_free(struct die d)
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

/** Determines if d is a boolean expression */
bool d_boolean(struct die *d)
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

/** Prints the given die expression, approximating the input format */
void d_print(struct die *d)
{
	switch(d->op)
	{
		case INT:
			printf("%d", d->constant);
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
			putchar('(');
			d_print(d->biop.l);
			printf(") %s (", tkstr(d->op));
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

/** Prints the syntax tree of the given die expression
	@param d a die expression
	@param depth how deep the current expression should be indented, i.e. how many indirections were taken
*/
void d_printTree(struct die *d, int depth)
{
	printIndent(depth);

	switch(d->op)
	{
		#define b(c,m) case c: printf("%s\n", m); d_printTree(d->biop.l, depth + 1); d_printTree(d->biop.r, depth + 1); break;

		case INT:
			printf("%u\n", d->constant);
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
