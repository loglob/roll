// settings.h: Contains the global settings struct that propagates the current CLI state.
#pragma once
#include <stdbool.h>


extern struct Settings
{
	enum
	{
		/** Dice should be analyzed and their value distribution plotted.
			Selected by -p
		*/
		PREDICT,
		/** Dice should be analyzed, and compared to a Gaussian distribution with the same µ and σ.
			Selected by -
		*/
		PREDICT_COMP_NORMAL,
		/** Dice should be analyzed, and compared to a reference die, stored in compare */
		PREDICT_COMP,
		/** Dice should be simulated a number of times stored in rolls */
		ROLL,
		/** Dice should be analyzed, and compared to the value stored in compareValue */
		COMPARE
	} mode;
	union
	{
		/** How often dice should be simulated, when mode is ROLL */
		int rolls;
		/** The distribution to compare dice against, when mode is PREDICT_COMP */
		struct Prob *compare;
		/** The value to compare dice against, when mode is COMPARE */
		int compareValue;
	};
	/** Whether to print debugging information. Set by -d */
	bool debug;
	/** Whether to print additional information when in ROLL mode. Set by -v */
	bool verbose;
	/** Whether to suppress histograms when in a PREDICT* mode. Set by -c */
	bool concise;

	/** The cutoff below which values are not displayed in histograms. Set by -t */
	double cutoff;
	/** If true, dynamically set cutoff such that at least one dot would be shown. Set by -td */
	bool dynamicCutoff;
	/** If true, apply the cutoff to any value, not just leasing and trailing values. Set by -g */
	bool globalCutoff;

	/** The amount of digits in displayed values. Set by -o */
	int precision;
	/** How many characters wide histograms should be. set by -w */
	int hcolOverwrite;

	/** If true, show only the probability of the given values in histograms. Set by -s */
	bool selectRange;
	/** The lowest histogram line to show if selectRange is true */
	int rLow;
	/** The highest histogram line to show if selectRange is true */
	int rHigh;

	/** The percentiles to check */
	int percentile;
} settings;
