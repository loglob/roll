/* Parses the language:
!! INT 'd'... is expanded to INT x d...
	this allows i.e. 2d20~1 to be interpreted as the more sensible 2x(1d20~1) instead of (2d20)~1

INT := [1-9][0-9]* ;
n := INT | 0+ | - INT ;

lim := n
	| ^
	| - ^
	| _
	| - _
;

range := lim
	| lim - lim
;

set := range
	| set , range
;

die := n
	| 'd' die
	| die ~ set
	| die ~ ! set
	| die \ set
	| die \ ! set
	| die ^ INT / INT
	| die ^ INT
	| die ^! INT
	| die _ INT / INT
	| die _ INT
	| die !
	| die $ INT
	| die $
	| die x die
	| die * die
	| die / die
	| die + die
	| die - die
	| die ^^ die
	| die __ die
	| die > die
	| die >= die
	| die < die
	| die >= die
	| die = die
	| die ? die
	| die ? die : die
	| ( die )
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
#include "ranges.h"
#include "die.h"

static const char mtok_str[][3] = { "^^", "__", "^!", "<=", ">=" };
static const char mtok_chr[] = { UPUP, __, UP_BANG, LT_EQ, GT_EQ };

/* represents the state of the lexer */
typedef struct lexState
{
	bool unlex;
	const char *pos, *err;
	char last;
	int num, pdepth;
} ls_t;

#pragma region Error Handling

/* A human-readable string representation of the given token.
	NOT reentrant. Return value is NOT safe after subsequent calls.  */
const char *tkstr(char tk)
{
	static char retBuf[2] = {};

	switch(tk)
	{
		case NUL: return "end of input";
		case INT: return "a positive number";
		case ZERO: return "zero";

		default:
            for (size_t i = 0; i < sizeof(mtok_chr) / sizeof(*mtok_chr); i++)
            {
                if(mtok_chr[i] == tk)
                    return mtok_str[i];
            }

            retBuf[0] = tk;
		return retBuf;
	}
}

/* Like _badtk, but accepts strings instead of chars. An empty string encodes the NUL token. */
static void _badtks(ls_t ls, const char *first, ...)
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
static void _badtk(ls_t ls, int first, ...)
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
		_badtks(ls, buf, "", NULL);
	else
		_badtks(ls, buf, NULL);
}


#define _LS *ls
#define lerrf(ls, fmt, ...) eprintf("Error at %.5s: " fmt "\n", (ls).err, ##__VA_ARGS__)
#define errf(fmt, ...) lerrf(_LS, fmt, ##__VA_ARGS__)
#define badtk(...) _badtk(_LS, __VA_ARGS__, -1)
#define badtks(...) _badtks(_LS, __VA_ARGS__, NULL)

#pragma endregion

#pragma region lexer functions

static char _lex(struct lexState *ls)
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
			errf("Integer value too large.\n");

		return ls->last = (ls->num) ? INT : ZERO;
	}
	if(!i)
	{
		ls->pos = ls->err;
		return ls->last = NUL;
	}
	if(strchr(SPECIAL, i))
	{
		int next;

		if((next = *ls->pos)) for(size_t m = 0; m < sizeof(mtok_chr); m++)
		{
			if(i == mtok_str[m][0] && next == mtok_str[m][1])
			{
				ls->pos++;
				return ls->last = mtok_chr[m];
			}
		}

		return ls->last = i;
	}

	errf("Unknown token: '%c'", i);
	__builtin_unreachable();
}

static int _lexi(struct lexState *ls)
{
	if(_lex(ls) != INT)
		badtk(INT);

	return ls->num;
}

static void _unlex(struct lexState *ls)
{
	if(ls->unlex)
		eprintf("Parsing failed: Double unlex\n");

	ls->unlex = true;
}

static bool _lexm(struct lexState *ls, char c)
{
	if(_lex(ls) == c)
		return true;
	else
	{
		_unlex(ls);
		return false;
	}
}

/* Retrieves the next token. Returns the read token kind. */
#define lex() _lex(ls)
/* Reads the next token abd asserts that it's a positive whole number.
	Returns the number read. */
#define lexi() _lexi(ls)
/* Causes the next lex() call to return the same token as the last.
	Only valid if lex() was called since the last call to unlex() */
#define unlex() _unlex(ls)
/* Reads the next token.
	Then checks if its kind is c and unlex()es it if it isn't.
	Returns whether the token matched. */
#define lexm(tk) _lexm(ls,tk)

#pragma endregion

#pragma region static functions
/** The operator precedence of the given operator.
	a lower precedence denotes operators that would be evaluated sooner.
	An even precedence indicates left- and odd indicates right association.
	*/
static int precedence(char op)
{
	switch(op)
	{
		case '?':
			return 20;
		case '<':
		case '>':
		case LT_EQ:
		case GT_EQ:
		case '=':
			return 10;
		case UPUP:
		case __:
			return 8;
		case '+':
			return 6;
		case '-':
			return 4;
		case '/':
			return 2;
		case '*':
			return 2;
		case 'x':
			return 1;
		default:
			return 0;
	}
}

/** Merges two expressions with a binary operator. Handles operator precedence. */
static struct die *d_merge(struct die *left, char op, struct die *right)
{
	int pl = precedence(left->op);
	int p = precedence(op);

	if(pl > p || (pl % 2 && pl >= p))
	{
		left->biop.r = d_merge(left->biop.r, op, right);
		return left;
	}
	else
		return d_clone((struct die){ .op = op, .biop = { .l = left, .r = right } });
}


static struct die *_parse_expr(ls_t *ls);

/** Parses an atom expression, so a number, a unary minus or a parenthesized expression.
	Also expands INT d into INT x d (see line 2) */
static inline struct die *_parse_atom(ls_t *ls)
{
	switch (lex())
	{
		case INT:
		case ZERO:
		{
			struct die *ret = d_clone((struct die){ .op = INT, .constant = ls->num });

			// peek forward
			ls_t bak = *ls;

			if(lexm('d'))
			{
				// insert a x token; see line 2
				*ls = bak;
				ls->last = 'x';
				unlex(); // <- set unlex flag
			}

			return ret;
		}

		case 'd':
			return d_clone((struct die){ .op = 'd', .unop = _parse_atom(ls) });

		case '(':
			ls->pdepth++;
			return _parse_expr(ls);

		case '-':
			return d_clone((struct die)
				{
					.op = '(',
					.unop = d_clone((struct die)
					{
						.op = '-',
						.biop =
						{
							.l = d_clone((struct die){ .op = INT, .constant = 0 }),
							.r = _parse_atom(ls)
						}
					})
				});

		default:
			badtk(INT, 'd', '(');
			__builtin_unreachable();
	}
}

/** Parses a range limit in a set filter */
static inline int _parse_lim(rl_t rng, ls_t *ls)
{
	int sgn = lexm('-') ? -1 : 1;

	switch(lex())
	{
		case '^':
			return sgn * rng.high;

		case '_':
			return sgn * rng.low;

		case INT:
			return sgn * ls->num;

		case ZERO:
			return 0;

		default:
			badtk('^', '_', '-', INT, ZERO);
			__builtin_unreachable();
	}
}

/** Iteratively parses every postfix unary operator.
	left is optional and represents the expression the postfix is applied to.
	Mutually recurses with _parse_expr() to parse parenthesized expressions. */
static inline struct die *_parse_pexpr(struct die *left, ls_t *ls)
{
	if(!left)
		left = _parse_atom(ls);

	for(;;)
	{
		char op;
		switch(op = lex())
		{
			case UP_BANG:
			case '^':
			case '_':
			{
				int sel = lexi();
				int of;

				if(lexm('/'))
					of = lexi();
				else
				{
					of = sel;
					sel = (op == UP_BANG) ? of - of/2 + !(of%2) : 1;
				}

				if(sel > of)
					errf("Invalid selection value: '%u/%u'", sel, of);

				left = d_clone((struct die){ .op = op, .select= { .v = left, .of = of, .sel = sel } });
			}
			continue;

			case '!':
				left = d_clone((struct die){ .op = op, .unop = left });
			continue;

			case '$':
				if(lexm(INT))
					left = d_clone((struct die){ .op = op, .explode = { .v = left, .rounds = ls->num } });
				else
					left = d_clone((struct die){ .op = op, .explode = { .v = left, .rounds = 1 } });
			continue;

			case '\\':
			case '~':
			{
				struct set set = { .negated = lexm('!') };
				rl_t rng = d_range(left);

				do
				{
					int start = _parse_lim(rng, ls);

					if(lexm('-'))
					{
						int end = _parse_lim(rng, ls);

						if(start > end)
							errf("Invalid range specifier, ranges must be ordered");

						set_insert(&set, start, end);
					}
					else
						set_insert(&set, start, start);

				} while(lexm(','));

				left = d_clone((struct die){ .op = op, .reroll = { .v = left, .set = set } });
			}
			continue;

			default:
				unlex();
			return left;
		}
	}
}

/** Iteratively parses every infix binary operator.
	Mutually recurses with _parse_pexpr() to parse parenthesized expressions. */
static struct die *_parse_expr(ls_t *ls)
{
	// the expression being constructed. Always a complete expression (i.e. not missing any leaves)
	struct die *left = _parse_pexpr(NULL, ls);

	for(;;)
	{
		char op = lex();

		if(op == NUL)
			return left;
		else if(strchr(BIOPS, op))
			left = d_merge(left, op, _parse_pexpr(NULL, ls));
		else if(op == ':' && left->op == '?')
		{
			struct die *c = left->biop.l;
			struct die *t = left->biop.r;

			left->op = ':';
			left->ternary.cond = c;
			left->ternary.then = t;
			left->ternary.otherwise = _parse_pexpr(NULL, ls);
		}
		else if(op == ')' && ls->pdepth)
		{
			ls->pdepth--;
			return d_clone((struct die){ .op = '(', .unop = left });
		}
		else if(ls->pdepth)
			badtks(BIOPS, UOPS, ")", "");
		else
			badtks(BIOPS, UOPS, "");
	}
}

#pragma endregion

/** Parses a die expression. Exits on parse failure. */
struct die *parse(const char *str)
{
	ls_t ls = (ls_t){ .pos = str };
	struct die *d = _parse_expr(&ls);

	if(ls.pdepth)
		_badtk(ls, ')', -1);

	return d;
}

#undef _LS
#undef lerrf
#undef errf
#undef badtk
#undef badtks
