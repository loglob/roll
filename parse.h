/* Parses the language:
!! INT 'd' INT... is expanded to INT x d INT...
	this allows i.e. 2d20~1 to be interpreted as the more sensible 2*(1d20~1) instead of (2d20)~1

intls := INT
	| intls , INT
;
expr := INT
	| 'd' INT
	| expr + expr
	| expr - expr
	| expr '*' expr
	| expr 'x' expr
	| expr !
	| expr $ INT
	| expr ^ INT / INT
	| expr _ INT / INT
	| expr ~ intls
	| expr \ intls
	| ( expr )
;
*/
#pragma once
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "util.h"

#define NUL ((char)0)
#define INT ((char)-2)
#define ZERO ((char)-3)
// equivalent to (char)-4
#define UPUP '\xFC'
// equivalent to (char)-3
#define __ '\xFB'
#define BIOPS "+-*x/<>" "\xFC\xFB"
#define SELECT "^_"
#define REROLLS "~\\"
#define UOPS SELECT REROLLS "!$"
#define SPECIAL BIOPS UOPS "d,/()?:"

/* represents the state of the lexer */
typedef struct lexstate
{
	bool unlex;
	const char *pos, *err;
	char last;
	int num, pdepth;
} ls_t;

/* represents a die expression as a syntax tree */
typedef struct dieexpr
{
	char op;
	union
	{
		// valid if op == ':'
		struct { struct dieexpr *cond, *then, *otherwise; } ternary;
		// valid if op in BIOPS
		struct { struct dieexpr *l, *r; } biop;
		// valid if op in SELECT
		struct { struct dieexpr *v; int sel, of; } select;
		// valid if op in REROLLS
		struct { struct dieexpr *v; int *ls; int count;} reroll;
		// valid if op is '$'
		struct { struct dieexpr *v; int rounds; } explode;
		// valid if op in UOPS
		struct dieexpr *unop;
		// valid if op == INT || op == d
		int constant;
	};
} d_t;

#pragma region Error Handling

/* A human-readable string representation of the given token.
	NOT reentrant. Return value is NOT safe after subsequent calls.  */
static const char *tkstr(char tk)
{
	static char retBuf[4];

	switch(tk)
	{
		case NUL: return "end of input";
		case INT: return "a positive number";
		case ZERO: return "zero";
		case UPUP: return "^^";
		case __: return "__";

		default:
			sprintf(retBuf, isalnum(tk) ? "'%c'" : "%c", tk);
		return retBuf;
	}
}


/* Like _unexptk, but accepts strings instead of chars. An empty string encoded the NUL token. */
static void _unexptks(ls_t ls, const char *first, ...)
{
	va_list vl;
	va_start(vl, first);

	fprintf(stderr, "Error at %.5s: Bad Token: Didn't expect %s; expected ", ls.err, tkstr(ls.last));

	for(const char *cur = first; cur;)
	{
		const char *next = va_arg(vl, const char*);

		if(!*cur)
		{ // NUL
			if(cur != first)
				fputs(next ? ", " : " or ", stderr);

			fputs(tkstr(NUL), stderr);
		}
		else for(;*cur; cur++)
		{
			if(cur != first)
				fputs((next || cur[1]) ? ", " : " or ", stderr);

			fputs(tkstr(*cur), stderr);
		}

		cur = next;
	}

	fputs(".\n", stderr);
	va_end(vl);
	exit(EXIT_FAILURE);
}

/* Prints an error message, describing that a token isn't in the given list of expected tokens. */
static void _unexptk(ls_t ls, int first, ...)
{
	va_list vl;
	va_start(vl, first);
	char buf[100];
	char *b = buf;

	bool nul = false;

	for (int cur = first; cur != -1; cur = va_arg(vl, int))
	{
		if(cur == NUL)
			nul = true;
		else
			*b++ = cur;
	}

	*b = 0;

	if(nul && b != buf)
		_unexptks(ls, buf, "", NULL);
	else
		_unexptks(ls, buf, NULL);
}


#define _LS *ls
#define lerrf(ls, fmt, ...) eprintf("Error at %.5s: " fmt "\n", (ls).err, __VA_ARGS__)
#define lerr(ls, msg) lerrf(ls, "%s", msg)
#define errf(fmt, ...) lerrf(_LS, fmt, __VA_ARGS__)
#define err(msg) lerr(_LS, msg)
#define lbadtk(ls, ...) _unexptk(ls, __VA_ARGS__, -1)
#define badtk(...) _unexptk(_LS, __VA_ARGS__, -1)
#define badtks(...) _unexptks(_LS, __VA_ARGS__, NULL)

#pragma endregion

#pragma region lexer functions

static char _lex(struct lexstate *ls)
{
	char i;

	if(ls->unlex)
	{
		ls->unlex = false;
		return ls->last;
	}

	if(!*ls->pos)
		return ls->last = NUL;

	do
		i = *ls->pos++;
	while(isspace(i));

	ls->err = ls->pos - 1;
	if(isdigit(i))
	{
		ls->num = strtoi(ls->pos - 1, (char**)&ls->pos, 10);

		// only ERANGE is possible
		if(errno)
			lerr(*ls, "Integer value too large.\n");

		return ls->last = (ls->num) ? INT : ZERO;
	}
	if(!i)
	{
		ls->pos = ls->err;
		return ls->last = NUL;
	}
	if(strchr(SPECIAL, i))
		return ls->last = i;

	lerrf(*ls, "Unknown token: '%c'", i);
	__builtin_unreachable();
}

static int _lexc(struct lexstate *ls, char c)
{
	char got = _lex(ls);

	if(got != c)
		lbadtk(*ls, c);

	return ls->num;
}

static void _unlex(struct lexstate *ls)
{
	if(ls->unlex)
		eprintf("Parsing failed: Double unlex\n");

	ls->unlex = true;
}

/* Retrieves the next token. Returns the read token kind. */
#define lex() _lex(ls)
/* Reads the next token.
	Then checks if its kind is c, prints an error message and exits if it isn't.
	Returns the read integer value. */
#define lexc(c) _lexc(ls, c)
/* Causes the next lex() call to return the same token as the last.
	Only valid if lex() was called since the last call to unlex() */
#define unlex() _unlex(ls)

#pragma endregion

#pragma region static functions
/* The operator precedence of the given operator.
	a lower precedence denotes operators that would be evaluated sooner. */
static int precedence(char op)
{
	switch(op)
	{
		case '?':
			return 20;
		case '<':
		case '>':
			return 10;
		case UPUP:
		case __:
			return 8;
		case '+':
			return 5;
		case '-':
			return 4;
		case '/':
			return 3;
		case '*':
			return 2;
		case 'x':
			return 1;
		default:
			return 0;
	}
}

/* Allocates a copy of the struct. */
static struct dieexpr *d_clone(struct dieexpr opt)
{
	struct dieexpr *val = xmalloc(sizeof(opt));
	*val = opt;
	return val;
}

/* Merges two expressions with a binary operator. Handles operator precedence. */
static struct dieexpr *d_merge(struct dieexpr *left, char op, struct dieexpr *right)
{
	int pl = precedence(left->op);
	int p = precedence(op);

	if(pl >= p)
	{
		left->biop.r = d_merge(left->biop.r, op, right);
		return left;
	}
	else
		return d_clone((struct dieexpr){ .op = op, .biop = { .l = left, .r = right } });
}


static struct dieexpr *_parse_expr(ls_t *ls);

/* Iteratively parses every postfix unary operator.
	left is optional and represents the expression the postfix is applied to.
	Mutually recurses with _parse_expr() to parse parenthesized expressions. */
static inline struct dieexpr *_parse_pexpr(struct dieexpr *left, ls_t *ls)
{
	if(!left)
	{
		switch (lex())
		{
			case INT:
			case ZERO:
			{
				left = d_clone((struct dieexpr){ .op = INT, .constant = ls->num });

				// peek forward
				ls_t bak = *ls;

				if(lex() == 'd')
				{
					// insert a x token; see line 2
					*ls = bak;
					ls->last = 'x';
					unlex();
					return left;
				}
				else
					unlex();
			}
			break;

			case 'd':
			{
				int pips = lexc(INT);
				left = d_clone((struct dieexpr){ .op = 'd', .constant = pips });
			}
			break;

			case '(':
			{
				ls->pdepth++;
				left = _parse_expr(ls);
			}
			break;

			default:
				badtk(INT, 'd', '(');
		}
	}

	for(;;)
	{
		char op;
		switch(op = lex())
		{
			case '^':
			case '_':
			{
				int nx = lex();

				if(nx == op)
				{
					ls->last = (op == '^') ? UPUP : __;
					unlex();
					return left;
				}
				else if(nx != INT)
					badtk(INT, op);

				int sel = ls->num;
				int of;

				if(lex() != '/')
				{
					unlex();
					of = sel;
					sel = 1;
				}
				else
					of = lexc(INT);

				if(sel >= of)
					err("Invalid selection values.");

				left = d_clone((struct dieexpr){ .op = op, .select= { .v = left, .of = of, .sel = sel } });
			}
			continue;

			case '!':
				left = d_clone((struct dieexpr){ .op = op, .unop = left });
			continue;

			case '$':
				if(lex() != INT)
				{
					unlex();
					left = d_clone((struct dieexpr){ .op = op, .explode = { .v = left, .rounds = 1 } });
				}
				else
					left = d_clone((struct dieexpr){ .op = op, .explode = { .v = left, .rounds = ls->num } });
			continue;

			case '\\':
			case '~':
			{
				int *v = NULL;
				int len = 0;

				do
				{
					v = xrealloc(v, (++len) * sizeof(int));

					switch(lex())
					{
						case '-':
							v[len - 1] = -lexc(INT);
						break;

						case INT:
							v[len - 1] = ls->num;
						break;

						default:
							badtk('-', INT);
						break;
					}
				} while (lex() == ',');

				left = d_clone((struct dieexpr){ .op = op, .reroll = { .v = left, .ls = v, .count = len } });
				unlex();
			}
			continue;

			default:
				unlex();
			return left;
		}
	}
}

/* Iteratively parses every infix binary operator.
	Mutually recurses with _parse_pexpr() to parse parenthesized expressions. */
static struct dieexpr *_parse_expr(ls_t *ls)
{
	// the expression being constructed. Always a complete expression (i.e. not missing any leaves)
	struct dieexpr *left = _parse_pexpr(NULL, ls);

	for(;;)
	{
		char op = lex();

		if(op == NUL)
			return left;
		else if(strchr(BIOPS, op) || op == '?')
			left = d_merge(left, op, _parse_pexpr(NULL, ls));
		else if(op == ':' && left->op == '?')
		{
			d_t *c = left->biop.l;
			d_t *t = left->biop.r;

			left->op = ':';
			left->ternary.cond = c;
			left->ternary.then = t;
			left->ternary.otherwise = _parse_pexpr(NULL, ls);
		}
		else if(op == ')' && ls->pdepth)
		{
			ls->pdepth--;
			return d_clone((d_t){ .op = '(', .unop = left });
		}
		else if(ls->pdepth)
			badtks(BIOPS, UOPS, ")", "");
		else
			badtks(BIOPS, UOPS, "");
	}
}

#pragma endregion

/* Parses a die expression */
struct dieexpr *parse(const char *str)
{
	ls_t ls = (ls_t){ .pos = str };
	struct dieexpr *d = _parse_expr(&ls);

	if(ls.pdepth)
		lbadtk(ls,')');

	return d;
}

/* frees all resources used by a die expression. */
void d_free(struct dieexpr *d)
{
	if(strchr(BIOPS, d->op))
	{
		d_free(d->biop.l);
		d_free(d->biop.r);
	}
	else if(strchr(SELECT, d->op))
		d_free(d->select.v);
	else if(strchr(REROLLS, d->op))
	{
		d_free(d->reroll.v);
		free(d->reroll.ls);
	}
	else if(strchr(UOPS, d->op))
		d_free(d->unop);

	free(d);
}

#undef _LS
#undef lerrf
#undef lerr
#undef errf
#undef err
#undef lbadtk
#undef badtk
