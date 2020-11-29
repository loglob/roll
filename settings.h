#pragma once
#include <stdbool.h>

struct
{
	enum
	{
		PREDICT,
		ROLL,
		COMPARE,
	} mode;
	int rolls;
	bool debug, verbose;

	double cutoff;
	bool dynamicCutoff;
	bool globalCutoff;

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