/* plotting.h: handles plotting of probability functions */
#pragma once
#include "prob.h"
#include "die.h"
#include "settings.h"
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

// contains data for plotting a function
struct plotInfo {
	// the maximum length of the preamble
	int prLen;
	// the preamble format string
	const char *preamble;
	// the amount of digits before the decimal in the percentages
	int floatLen;
	// the amount of bars to draw per percent
	double scaling;
	// The cutoff below which values aren't displayed
	double cutoff;
};

#pragma region internal functions

/* Retrieves the total width of the terminal */
static unsigned int hcol()
{
	if(settings.hcolOverwrite)
		return settings.hcolOverwrite;

	const char *env = getenv("COLUMNS");

	if(env)
		return strtoui(env, NULL, 10);

	struct winsize w;
	if(!ioctl(STDOUT_FILENO, TIOCGWINSZ, &w))
		return w.ws_col;

	return 200;
}

/* Determines the width of a number if printed in base10 */
static inline int numw(signed int n)
{
	return n ? ((n < 0) + 1 + floor(log10((double)abs(n)))) : 1;
}

/** initializes a plotInfo structure for the given properties.
	@param preamble A printf format to be printed before each row.
	@param prlen The max length of the preamble. A preamble is padded to at least this length.
	@param pmax The maximum probability in the plotted data.
	@returns A structure that contains information on how to format plotted data
*/
static struct plotInfo plot_init(const char *preamble, int prlen, double pmax)
{
	struct plotInfo pi = {
		.preamble = preamble,
		.prLen = prlen,
		.cutoff = settings.cutoff
	};

	double precFact = pow(10, settings.precision);

	pi.floatLen = numw((int)floor(round(pmax * 100 * precFact) / precFact));

	int plotArea = hcol()
	// remove preamble
	 - prlen
	// ": "
	 - 2
	// 00
	 - pi.floatLen
	// .000*
	- 1 - settings.precision
	// "% "
	- 2;

	pi.scaling = plotArea / pmax;

	if(settings.dynamicCutoff)
		pi.cutoff = 1 / pi.scaling;

	return pi;
}

/** Outputs the preamble before the beginning of a row
	@param pi returned from plot_init()
	@param p The probability of this row
	@param ... The arguments to the format string passed in to plot_init
	@returns Whether this row was actually included in the output,
		instead of being trimmed due to cutoff settings.
*/
static bool plot_preamble(struct plotInfo pi, double p, ...)
{
	va_list l;
	va_start(l, p);

	if(!settings.globalCutoff || p > pi.cutoff)
	{
		int prlen = vprintf(pi.preamble, l);
		printf(": %*.*f%% ",
			// (pad preamble)    00            .   000*
			(pi.prLen - prlen) + pi.floatLen + 1 + settings.precision,
			settings.precision, 100 * p);

		va_end(l);
		return true;
	}
	else
	{
		va_end(l);
		return false;
	}
}

/** Prints a bar of a plot.
	@param pi obtained from plot_init().
	@param p the current probability.
*/
static void plot_bar(struct plotInfo pi, double p)
{
	int barLen = (int)round(p * pi.scaling);

	for (int i = 0; i < barLen; i++)
		putchar('#');

	putchar('\n');
}

/** Prints a bar of a plot in compare mode.
	Uses '+' and '-' in place of '#' to indicate differences.
	@param pi obtained from plot_init()
	@param p the current probability
	@param e the expected probability from the distribution being compared
 */
static void plot_barC(struct plotInfo pi, double p, double e)
{
	int barLen = (int)round(p * pi.scaling);
	int expLen = (int)round(e * pi.scaling);

	for (int i = 0; i < barLen || i < expLen; i++)
	{
		if(i >= barLen)
			putchar('-');
		else if(i >= expLen)
			putchar('+');
		else
			putchar('#');
	}

	putchar('\n');
}

/** Plots the difference between two probability functions
	@param p the current probability function
	@param e the expected probability function
 */
void plot_diff(struct prob p, struct prob e)
{
	double sumErr = 0, sumSqErr = 0, sumRelErr = 0, sumSqRelErr = 0;
	int lowest = min(p.low, e.low);
	int samples = max(p_h(p), p_h(e)) - lowest + 1;

	for (int n = lowest; n < lowest + samples; n++)
	{
		double x = probof(p ,n), y = probof(e, n);
		double d = fabs(x - y);

		sumErr += d;
		sumSqErr += d*d;

		if(d > 0 && y > 0)
		{
			double r = d / y;

			sumRelErr += r;
			sumSqRelErr += r * r;
		}
	}

	// TODO: (MAYBE) Better algorithm for comparing against normal distribution that integrates numerically outside of the die's range
	printf("Total error: %2$.*1$f%%	Mean error: %3$.*1$f%%\n", settings.precision,
						100 * sumErr,		100 * sumErr / samples);
	printf("Total Relative error: %2$.*1$f%%	Mean Relative error: %3$.*1$f%%\n", settings.precision,
								100 * sumRelErr,			100 * sumRelErr / samples);
	printf("Total Squared error: %2$.*1$f%%	Mean Squared error: %3$.*1$f%%	Root-Mean-Square error: %4$.*1$f%%\n",
		settings.precision, 	100 *sumSqErr, 		100 * sumSqErr / samples,			100 * sqrt(sumSqErr / samples));
	printf("Total Squared Relative error: %2$.*1$f%%	Mean Squared Relative error: %3$.*1$f%%	Root-Mean-Square-Relative error: %4$.*1$f%%\n",
		settings.precision, 			100 * sumSqRelErr, 					100 * sumSqRelErr / samples,		100 * sqrt(sumSqRelErr/samples));
}

/** Trims p according to program arguments. In place.
	@param p A probability function
	@param pi obtained from plot_init()
	@returns p without trimmed values, as per program settings.
		References the same array as the input.
*/
static struct prob p_trims(struct prob p, struct plotInfo pi)
{
	// left and right offsets
	int start, end;

	for (start = 0; p.p[start] < pi.cutoff && start < p.len; start++);
	for (end = 0; p.p[p.len - 1 - end] < pi.cutoff && end < p.len; end++);

	if(settings.selectRange)
	{
		start = max(start, settings.rLow - p.low);
		end = max(end, p_h(p) - settings.rHigh);
	}

	return (struct prob){ .len = p.len - start - end, .low = p.low + start, .p = p.p + start };
}

#pragma endregion

/** Plots p onto stdout.
	@param p A probability function
*/
void p_plot(struct prob p)
{
	int mw = max(numw(p.low), numw(p_h(p)));
	double pmax = p.p[0];

	if(settings.mode == PREDICT_COMP && settings.compare && settings.compare->p[0] > pmax)
		pmax = settings.compare->p[0];

	for (int i = 1; i < p.len; i++)
	{
		if(p.p[i] > pmax)
			pmax = p.p[i];

		if(settings.mode == PREDICT_COMP && settings.compare)
		{
			double c = probof(*settings.compare, i + p.low);

			if(c > pmax)
				pmax = c;
		}
	}

	struct plotInfo pi = plot_init("%*d", mw, pmax);
	p = p_trims(p, pi);

	if(settings.mode == PREDICT_COMP && settings.compare)
	{
		struct prob c = p_trims(*settings.compare, pi);
		int hi = max(p_h(p), p_h(c));

		for (int n = min(p.low, c.low); n <= hi; n++)
		{
			double x = probof(p, n);

			if(plot_preamble(pi, x, mw, n))
				plot_barC(pi, x, probof(c, n));
		}
	}
	else for (int i = 0; i < p.len; i++)
	{
		if(plot_preamble(pi, p.p[i], mw, i + p.low))
			plot_bar(pi, p.p[i]);
	}
}

/** Prints a header describing a probability function
	@param p A probability function
	@param mu If not NULL, overwritten with the expected value of p
	@param sigma If not NULL, overwritten with the standard deviation of p
 */
void p_header(struct prob p, double *mu, double *sigma)
{
	double avg = 0.0;
	double var = 0.0;
	double sum = 0.0;

	double pL = 0.0;
	double pH = 0.0;
	double lp = settings.percentile / 100.0;
	double hp = (100 - settings.percentile) / 100.0;

	for (int i = 0; i < p.len; i++)
	{
		avg += p.p[i] * (i + p.low);

		// intra-bucket linear interpolation
		if(sum < lp && sum + p.p[i] >= lp)
			pL = (p.low + i) + (lp - sum) / p.p[i];
		if(sum < hp && sum + p.p[i] >= hp)
			pH = (p.low + i) + (hp - sum) / p.p[i];

		sum += p.p[i];
	}
	for (int i = 0; i < p.len; i++)
	{
		double d = (i + p.low) - avg;
		var += d * d * p.p[i];
	}

	printf("Avg: %f\tVariance: %f\tSigma: %f\n", avg, var, sqrt(var));
	printf("Min: %d\t %u%%: %f\t %u%%: %f\tMax: %d\n", p.low, settings.percentile, pL, 100 - settings.percentile, pH, p_h(p));

	if(mu)
		*mu = avg;
	if(sigma)
		*sigma = sqrt(var);
}

/** Prints a boolean (i.e. 0/1) probability function.
	@param p A probability function
*/
void p_printB(struct prob p)
{
	const char *strs[] = { "false", "true" };
	bool isConst = p.len == 1;
	struct plotInfo pi = plot_init("%s", 5, isConst ? 1.0 : p.p[p.p[0] < p.p[1]]);

	for (int i = 0; i <= 1; i++)
	{
		double x = isConst ? p.low == i : p.p[i];
		if(plot_preamble(pi, x, strs[i]))
			plot_bar(pi, x);
	}
}

/** Plots p's compare mode against settings.compareValue
	@param p a probability function
	@param to The value to compare against
*/
void p_comp(struct prob p, int to)
{
	double cpr[5] = {};
	const char *op[] = { "<= ", " < ", " = ", " > ", ">= " };

	for (int i = 0; i < p.len; i++)
	{
		if(p.low + i > to)
			cpr[3] += p.p[i];
		else if(p.low + i < to)
			cpr[1] += p.p[i];
		else
			cpr[2] += p.p[i];
	}

	cpr[0] = cpr[1] + cpr[2];
	cpr[4] = cpr[3] + cpr[2];
	double pmax = (cpr[0] > cpr[4]) ? cpr[0] : cpr[4];

	struct plotInfo pi = plot_init("%s%d", 3 + numw(to), pmax);

	for (int i = 0; i < 5; i++)
	{
		if(plot_preamble(pi, cpr[i], op[i], to))
			plot_bar(pi, cpr[i]);
	}
}

/** Prints debug info on p
	@param p a probability function
*/
void p_debug(struct prob p)
{
	double sum = 0;
	bool a0 = true;

	for (int i = 0; i < p.len; i++)
	{
		sum += p.p[i];
		a0 = a0 && p.p[i] >= 0.0;
	}

	printf("Sum: %.*f%%\n", settings.precision, sum * 100);

	if(!a0)
		printf("Function breaks axiom (0): Its Image is not a subset of ℚ⁺\n");
	if(sum < 0.9999995 || sum >= 1.0000005)
		printf("Function breaks axiom (1): The of sum its image isn't 1.\n");
	if(p.p[0] == 0)
		printf("Function breaks axiom (2): p(p_ℓ) = 0\n");
	if(p.p[p.len - 1] == 0)
		printf("Function breaks axiom (3): p(p_h) = 0\n");
}
