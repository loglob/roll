// die.h: Defines the die struct and type.
#pragma once
#include <stdbool.h>
#include "ast.h"
#include "set.h"
#include "prob.h"

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
// equivalent to (char)-9
#define NEQ '\xF7'
// equivalent to (char)-10
#define UP_DOLLAR '\xF6'
// equivalent to (char)-11
#define DOLLAR_UP '\xF5'
#define RELOPS "<>=\xF9\xF8\xF7"
#define BIOPS "+-*x/?" RELOPS "\xFC\xFB"
#define SELECT "^_\xFA\xF6\xF5"
#define REROLLS "~\\"
#define UOPS SELECT REROLLS "!$d("
#define SPECIAL BIOPS UOPS ",/():;[]"

#define EXPLODE_RATIO 2


/** Frees all resources used by a die expression. */
void d_free(struct die die);
void d_freeP(struct die *die);
struct die *d_clone(struct die d);

/** Determines if d is a boolean expression */
bool d_boolean(struct die *d);

/** Prints the given die expression, approximating the input format */
void d_print(struct die *d);

/** Prints the syntax tree of the given die expression
	@param d a die expression
	@param depth how deep the current expression should be indented, i.e. how many indirections were taken
*/
void d_printTree(struct die *d, int depth);

/* Transforms a dice expression to equivalent probability function. */
struct prob translate(struct die *d);