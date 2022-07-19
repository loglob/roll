/* plotting.h: handles plotting of p_t functions */
#pragma once
#include "prob.h"
#include "parse.h"
#include "settings.h"
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
bool d_boolean(d_t *d)
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
	pmax is the maximum probability in the plotted data. */
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

static bool plot_preamble(struct plotinfo pi, double p, ...)
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

		va_end(l);
		return true;
	}
	else
	{
		va_end(l);
		return false;
	}
}

/* Prints a row of the plot.
	pi is obtained from plot_init().
	p is the current probability.
	the following are arguments for the preamble format. */
static void plot_bar(struct plotinfo pi, double p)
{
	int barlen = (int)round(p * pi.scaling);

	for (int i = 0; i < barlen; i++)
		putchar('#');

	putchar('\n');
}

static void plot_barC(struct plotinfo pi, double p, double e)
{
	int barlen = (int)round(p * pi.scaling);
	int explen = (int)round(e * pi.scaling);

	for (int i = 0; i < barlen || i < explen; i++)
	{
		if(i >= barlen)
			putchar('-');
		else if(i >= explen)
			putchar('+');
		else
			putchar('#');
	}

	putchar('\n');
}

static void plot_diff(struct prob p, struct prob e)
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
		end = max(end, p_h(p) - settings.rHigh);
	}

	return (struct prob){ .len = p.len - start - end, .low = p.low + start, .p = p.p + start };
}

#pragma endregion

/* Plots p onto stdout. */
void p_plot(struct prob p)
{
	int mw = max(numw(p.low), numw(p_h(p)));
	double pmax = p.p[0];

	if(settings.compare && settings.compare->p[0] > pmax)
		pmax = settings.compare->p[0];

	for (int i = 1; i < p.len; i++)
	{
		if(p.p[i] > pmax)
			pmax = p.p[i];

		if(settings.compare)
		{
			double c = probof(*settings.compare, i + p.low);

			if(c > pmax)
				pmax = c;
		}
	}

	struct plotinfo pi = plot_init("%*d", mw, pmax);
	p = p_trim(p, pi);

	if(settings.compare)
	{
		p_t c = p_trim(*settings.compare, pi);
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

void p_printB(struct prob p)
{
	const char *strs[] = { "false", "true" };
	bool isConst = p.len == 1;
	struct plotinfo pi = plot_init("%s", 5, isConst ? 1.0 : p.p[p.p[0] < p.p[1]]);

	for (int i = 0; i <= 1; i++)
	{
		double x = isConst ? p.low == i : p.p[i];
		if(plot_preamble(pi, x, strs[i]))
			plot_bar(pi, x);
	}
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
	{
		if(plot_preamble(pi, cpr[i], op[i], settings.compareValue))
			plot_bar(pi, cpr[i]);
	}
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
