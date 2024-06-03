// ast.h: Contains the definition for the AST types die and pattern
#pragma once
#include "set.h"

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
		struct { struct die *v; int sel, of, bust; } select;
		/** valid if op in REROLLS*/
		struct
		{
			/** The die to roll on */
			struct die *v;
			/** The results to reroll */
			struct set set;
		} reroll;
		// valid if op is '$'
		struct { struct die *v; int rounds; } explode;
		// valid if op is '['
		struct {
			struct die *v;
			/** The length of patterns and actions */
			int cases;
			/** The patterns that are matched against */
			struct pattern *patterns;
			/** The dice rolled when the corresponding pattern matches.
				May be NULL to indicate a pattern check rather than match. */
			struct die *actions;
		} match;
		// valid if op in UOPS
		struct die *unop;
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
struct pattern
{
	/** The comparison operator, or 0 if this is a set pattern */
	char op;
	union
	{
		/** A set to match against. Valid if op is 0 */
		struct set set;
		/** A die to compare against, if op is in RELOPS */
		struct die die;
	};
};
