// translate.h: Contains routines for translating syntax trees into probability functions
#pragma once
#include "ast.h"
#include "prob.h"
#include <stdbool.h>

struct ProbCtx
{
	union {
		/** Owned by the context, and may be transferred to the `translate` callee  */
		const struct Prob prob;
		const int val;
	};
	/** If true, use `val` instead of `ctx` */
	const bool singleton;
	/** If true, `ctx` has been returned at least once from `translate`.
		Always false if `singleton` is true.
	 */
	bool consumed;
};

#define CONST_CTX(x) ((struct ProbCtx){ .val = (x), .singleton = true })

struct ProbCtx initCtx(struct Prob p);
void freeCtx(struct ProbCtx ctx);

/** Transforms a dice expression to equivalent probability function. */
struct Prob translate(struct ProbCtx *ctx, const struct Die *d);

/** Translates pattern for probability checking. */
struct PatternProb pt_translate(struct ProbCtx *ctx, struct Pattern p);
