/* plotting.h: handles plotting of p_t functions */
#pragma once
#include "prob.h"
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

// contains data for plotting a function
struct plotinfo {
	// the maximum length of the preamble
	int prlen;
	// the preamble format string
	const char *preamble;
	// the amount of digits before the decimal in the percentages
	int floatlen;
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

/* Determines if d is a boolean expression */
static bool d_boolean(d_t *d)
{
	switch(d->op)
	{
		case '<':
		case '>':
			return true;

		default:
			return false;
	}
}

/* Determines the width of a number if printed in base10 */
static inline int numw(signed int n)
{
	return n ? ((n < 0) + 1 + floor(log10((double)abs(n)))) : 1;
}

/* initializes a plotinfo structure for the given properties.
	preamble is a printf format to be printed before each row.
	prlen is the max length of the preamble.
		A preamble is padded to at least this length.
	pmax is the maximum probability in the plotted. */
static struct plotinfo plot_init(const char *preamble, int prlen, double pmax)
{
	struct plotinfo pi = {
		.preamble = preamble,
		.prlen = prlen,
		.cutoff = settings.cutoff
	};

	double pfact = pow(10, settings.precision);

	pi.floatlen = numw((int)floor(round(pmax * 100 * pfact) / pfact));

	int plotarea = hcol()
	// remove preamble
	 - prlen
	// ": "
	 - 2
	// 00
	 - pi.floatlen
	// .000*
	- 1 - settings.precision
	// "% "
	- 2;

	pi.scaling = plotarea / pmax;

	if(settings.dynamicCutoff)
		pi.cutoff = 1 / pi.scaling;

	return pi;
}

/* Prints a row of the plot.
	pi is obtained from plot_init().
	p is the current probability.
	the following are arguments for the preamble format. */
static void plot_row(struct plotinfo pi, double p, ...)
{
	va_list l;
	va_start(l, p);

	if(!settings.globalCutoff || p > pi.cutoff)
	{
		int prlen = vprintf(pi.preamble, l);
		printf(": %*.*f%% ",
			// (pad preamble)    00            .   000*
			(pi.prlen - prlen) + pi.floatlen + 1 + settings.precision,
			settings.precision, 100 * p);

		int barlen = (int)round(pi.scaling * p);

		for (int i = 0; i < barlen; i++)
			putchar('#');

		putchar('\n');
	}

	va_end(l);
}

/* trims p according to program arguments.
	The returned function references the same array as the input. */
static struct prob p_trim(struct prob p, struct plotinfo pi)
{
	// left and right offsets
	int start, end;

	for (start = 0; p.p[start] < pi.cutoff && start < p.len; start++);
	for (end = 0; p.p[p.len - 1 - end] < pi.cutoff && end < p.len; end++);

	if(settings.selectRange)
	{
		start = max(start, settings.rLow - p.low);
		end = max(end, (p.low + p.len - 1) - settings.rHigh);
	}

	return (struct prob){ .len = p.len - start - end, .low = p.low + start, .p = p.p + start };
}

#pragma endregion

/* Plots p onto stdout. */
void p_plot(struct prob p)
{
	int mw = max(numw(p.low), numw(p_h(p)));
	double pmax = p.p[0];

	for (int i = 1; i < p.len; i++)
	{
		if(p.p[i] > pmax)
			pmax = p.p[i];
	}

	struct plotinfo pi = plot_init("%*d", mw, pmax);
	p = p_trim(p, pi);

	for (int i = 0; i < p.len; i++)
		plot_row(pi, p.p[i], mw, i + p.low);
}

/* Prints full info on p to stdout. */
void p_print(struct prob p)
{
	double avg = 0.0;
	double var = 0.0;
	double p25 = 0.0;
	double p75 = 0.0;
	double sum = 0.0;

	for (int i = 0; i < p.len; i++)
	{
		avg += p.p[i] * (i + p.low);

		// intra-bucket linear interpolation
		if(sum < 0.25 && sum + p.p[i] >= 0.25)
			p25 = (p.low + i) + (.25 - sum) / p.p[i];
		if(sum < 0.75 && sum + p.p[i] >= 0.75)
			p75 = (p.low + i) + (.75 - sum) / p.p[i];
	
		sum += p.p[i];
	}
	for (int i = 0; i < p.len; i++)
	{
		double d = (i + p.low) - avg;
		var += d * d * p.p[i];
	}

	printf("Avg: %f\tVariance: %f\tSigma: %f\n", avg, var, sqrt(var));
	printf("Min: %d\t 25%%: %f\t 75%%: %f\tMax: %d\n", p.low, p25, p75, p.low + p.len - 1);

	if(!settings.concise)
		p_plot(p);
}

void p_printB(struct prob p)
{
	const char *strs[] = { "false", "true" };
	bool isConst = p.len == 1;
	struct plotinfo pi = plot_init("%s", 5, isConst ? 1.0 : p.p[p.p[0] < p.p[1]]);

	for (int i = 0; i <= 1; i++)
		plot_row(pi, isConst ? p.low == i : p.p[i], strs[i]);
}

/* Plots p's compare mode */
void p_comp(struct prob p)
{
	double cpr[5] = {};
	const char *op[] = { "<= ", " < ", " = ", " > ", ">= " };

	for (int i = 0; i < p.len; i++)
	{
		if(p.low + i > settings.compareValue)
			cpr[3] += p.p[i];
		else if(p.low + i < settings.compareValue)
			cpr[1] += p.p[i];
		else
			cpr[2] += p.p[i];
	}

	cpr[0] = cpr[1] + cpr[2];
	cpr[4] = cpr[3] + cpr[2];
	double pmax = (cpr[0] > cpr[4]) ? cpr[0] : cpr[4];

	struct plotinfo pi = plot_init("%s%d", 3 + numw(settings.compareValue), pmax);

	for (int i = 0; i < 5; i++)
		plot_row(pi, cpr[i], op[i], settings.compareValue);
}

/* Prints debug info on p */
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
		printf("Function breaks axiom (0)\nIts Image is not a subset of ℚ⁺\n");
	if(sum < 0.9999995 || sum >= 1.0000005)
		printf("Function breaks axiom (1)\nThe sum its image isn't 1.\n");
	if(p.p[0] == 0)
		printf("Function breaks axiom (2)\np(p_ℓ) = 0\n");
	if(p.p[p.len - 1] == 0)
		printf("Function breaks axiom (3)\np(p_h) = 0\n");
}
