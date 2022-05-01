#pragma once
#include <stdbool.h>

struct
{
	enum
	{
		PREDICT,
		PREDICT_COMP_NORMAL,
		PREDICT_COMP,
		ROLL,
		COMPARE
	} mode;
	int rolls;
	bool debug, verbose, concise;

	double cutoff;
	bool dynamicCutoff;
	bool globalCutoff;
	struct prob *compare;

	int precision;
	int hcolOverwrite;

	int compareValue;

	bool selectRange;
	int rLow, rHigh;
} settings =
{
	.mode = PREDICT,
	.cutoff = 0.000005,
	.precision = 3
};