// die.h: Defines the die struct and type.
#pragma once
#include "ast.h"
#include "prob.h"
#include <stdbool.h>

#define EXPLODE_RATIO 2

/* Transforms a dice expression to equivalent probability function. */
struct Prob translate(struct Prob *ctx, const struct Die *d);

/* Translates pattern for probability checking. */
struct PatternProb pt_translate(struct Prob *ctx, struct Pattern p);
