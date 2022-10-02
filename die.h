// die.h: Defines the die struct and type.
#pragma once
#include "set.h"

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
#define BIOPS "+-*x/" RELOPS "\xFC\xFB"
#define SELECT "^_\xFA"
#define REROLLS "~\\"
#define UOPS SELECT REROLLS "!$d("
#define SPECIAL BIOPS UOPS ",/()?:"

extern const char *tkstr(char tok);

/* represents a die expression as a syntax tree */
struct die
{
	char op;
	union
	{
		// valid if op == ':'
		struct { struct die *cond, *then, *otherwise; } ternary;
		// valid if op in BIOPS
		struct { struct die *l, *r; } biop;
		// valid if op in SELECT
		struct { struct die *v; int sel, of; } select;
		/** valid if op in REROLLS*/
		struct
		{
			/** The die to roll on */
			struct die *v;
			/** The results to reroll */
			struct set set;
			/** If true, negates selection such that every value _not_ in set is rerolled. */
			bool neg;
		} reroll;
		// valid if op is '$'
		struct { struct die *v; int rounds; } explode;
		// valid if op in UOPS
		struct die *unop;
		// valid if op == INT
		int constant;
	};
};
