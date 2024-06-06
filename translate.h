// translate.h: Contains routines for translating syntax trees into probability functions
#pragma once
#include "ast.h"
#include "prob.h"
#include <stdbool.h>

/** Transforms a dice expression to equivalent probability function. */
struct Prob translate(const struct Prob *ctx, const struct Die *d);

/** Translates pattern for probability checking. */
struct PatternProb pt_translate(const struct Prob *ctx, struct Pattern p);
