// ast.h: Contains the definition for the AST types die and pattern
#pragma once
#include "set.h"
#include "util.h"


#define NUL ((char)0)
/** non-utf8 char that marks an integer token */
#define INT ((char)-2)
/** non-utf8 char that marks a zero */
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
// equivalent to (char)-9
#define NEQ '\xF7'
// equivalent to (char)-10
#define UP_DOLLAR '\xF6'
// equivalent to (char)-11
#define DOLLAR_UP '\xF5'
/** All valid relational operators (also binary operators) */
#define RELOPS "<>=\xF9\xF8\xF7"
/** All valid binary operators */
#define BIOPS "+-*x/?" RELOPS "\xFC\xFB"
/** All valid unary operators */
#define SELECT "^_\xFA\xF6\xF5"
/** All valid reroll operators */
#define REROLLS "~\\"
/** All valid unary operators */
#define UOPS SELECT REROLLS "!$d("
/** All valid values for `Die.op` */
#define OPS BIOPS UOPS "@[:"

/* represents a die expression as a syntax tree */
struct Die
{
	char op;
	union
	{
		// valid if op == ':'
		struct { struct Die *cond, *then, *otherwise; } ternary;
		// valid if op in BIOPS
		struct { struct Die *l, *r; } biop;
		// valid if op in SELECT
		struct { struct Die *v; int sel, of, bust; } select;
		/** valid if op in REROLLS*/
		struct
		{
			/** The die to roll on */
			struct Die *v;
			/** The results to reroll */
			struct Set set;
		} reroll;
		// valid if op is '$'
		struct { struct Die *v; int rounds; } explode;
		// valid if op is '['
		struct {
			struct Die *v;
			/** The length of patterns and actions */
			int cases;
			/** The patterns that are matched against */
			struct Pattern *patterns;
			/** The dice rolled when the corresponding pattern matches.
				May be NULL to indicate a pattern check rather than match. */
			struct Die *actions;
		} match;
		// valid if op in UOPS
		struct Die *unop;
		// valid if op == INT
		int constant;
	};
};

/* Patterns are one of:
	rel := '<' | '>' | '>=' | '<=' | '=' | '/=' ;

	P := T
		| F
		| SET
		| rel DIE
	;

	MATCHES := P DIE
		| MATCHES ';' MATCHES
	;

	MATCH := '[' MATCHES ']' ;
*/
struct Pattern
{
	/** The comparison operator, or 0 if this is a set pattern */
	char op;
	union
	{
		/** A set to match against. Valid if op is 0 */
		struct Set set;
		/** A die to compare against, if op is in RELOPS */
		struct Die die;
	};
};

CLONE_SIG(d_clone, struct Die);

/** Frees all resources used by a die expression. */
void d_free(struct Die die);
void d_freeP(struct Die *die);

/** Prints the given die expression, approximating the input format */
void d_print(struct Die *d);

/** Prints the syntax tree of the given die expression
	@param d a die expression
	@param depth how deep the current expression should be indented, i.e. how many indirections were taken
*/
void d_printTree(struct Die *d, int depth);


void pt_free(struct Pattern pt);
void pt_print(struct Pattern p);
void pt_freeP(struct Pattern *p);

