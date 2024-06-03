#define _POSIX_C_SOURCE 200809L
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "parse.h"
#include "plotting.h"
#include "prob.h"
#include "settings.h"
#include "sim.h"
#include "xmalloc.h"


struct Settings settings =
{
	.mode = ROLL,
    .rolls = 1,
	.cutoff = 0.000005,
	.precision = 3,
	.percentile = 25
};


int main(int argc, char **argv)
{
	if(argc == 1)
		goto print_help;

	srand(clock());

	for (int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
		{
			switch(argv[i][1])
			{
				case 'h':
				case 'H':
				print_help:
					fprintf(stderr,
						"%1$s  Generates dice histograms\n"
						"Usage:\n"
						"	%1$s [arguments|dice]...\n"
						"Arguments:\n"
						"	-h       prints this help page\n"
						"	-d       Prints debug info\n"
						"	-v       Enables additional printout when using -r. Specify again to negate.\n"
						"	-q       Don't print any histograms for -p, -n or -a. Specify again to negate.\n"
						"	-t[c=0]  Sets the minimum percentage to display in histograms. Specify nothing to disable trimming.\n"
						"	-td      Like -t[c], but sets the minimum value so that it shows at least one histogram dot.\n"
						"	-tg      Applies the cutoff of -t to every value, not just starting and ending values.\n"
						"	-s[a..b] Shows only the given range of values in histograms.\n"
						"	-o[p]    Sets the output precision for floats. Overwrites -t with the minimum displayable value.\n"
						"	-w[n]    Sets the width of output.\n"
						"	-%%n      Also calculates the nth percentile of a dice expression in -p, -n or -a mode.\n"
						" Mode arguments:\n"
						"	-r[n=1]  Simulates a dice expression n times. (default)\n"
						"	-p       Prints an analysis and a histogram for a dice expression.\n"
						"	-c[v]    Compares a dice expression to a number.\n"
						"	-n       Compares the result percentage to a normal distribution with the same 𝜇 and 𝜎.\n"
						"	         	Note that the squared error values are slightly overestimated.\n"
						"	-a       Compares the first given die to all following dice.\n"
						"These modes are applied to all following dice, until another mode is specified.\n"
						"The default mode is -p\n"
						"Dice:\n"
						" A die is represented by the language:\n"
						"	n d m  Expands to 'n x d m'.\n"
						"	d n    Rolling a die with n sides.\n"
						"	@      The actual result of the last successful pattern match.\n"
						"	n      A constant value of n. n may be 0.\n"
						"	D~F    Rerolls once if any of the given values are rolled.\n"
						"	D\\F   Like ~ with infinite rerolls.\n"
						"	D^n/m  Selects the n highest values out of m tries.\n"
						"	D_n/m  Selects the n lowest values out of m tries.\n"
						"	D^!n/m/k Like D^n/m, but returns D's minimum minus 1 if k or more values roll their minimum.\n"
						"	D^!n/m Like D^!n/m/k but goes bust if half or more values roll their minimum.\n"
						"	D^$n/m/k Like D^!n/m/k, but also explodes dice once for every two rolls of their maximum in the total (before selection) pool\n"
						"	D^$n/m Like D^!n/m for D^$n/m/k.\n"
						"	D$^n/m Roll m dice and selects the n highest. Then adds an additional roll for every two rolls of the maximum in the pool of m dice.\n"
						"	D^n    Identical to D^1/n.\n"
						"	D_n    Identical to D_1/n.\n"
						"	D^!n   Identical to D^!1/n.\n"
						"	D^$n   Identical to D^$1/n.\n"
						"	D$^n   Identical to D$^1/n.\n"
						"	D!     Rolls D with exploding dice, so another roll of D is added to the maximum value\n"
						"          	and another roll of D is subtracted from the minimum value.\n"
						"	D$n    Like D! but only allows explosions, not implosions, so only maximum rolls are affected.\n"
						"	       	Additionally, n specifies how many rounds of explosions are permitted.\n"
						"	D$     Identical to D$1.\n"
						"	D[pt]  Rolls on a die and checks whether the roll matches any given pattern, separated by ';'.\n"
						"	        Each pattern may be followed by ':' and a die. That die is rolled when that pattern is hit.\n"
						"	        That expression may use '@' to access the matched value.\n"
						"	        If patterns don't have a die attached, returns whether any pattern matched. Otherwise, discard the roll if no patterns are hit.\n"
						"	DxD    Rolls on the left die, then adds that many rolls of the right die.\n"
						"	D*D    Rolls on both dice, then multiplies the results.\n"
						"	D/D    Rolls on both dice, then divides the results.\n"
						"	D+D    Adds the results of two dice.\n"
						"	D-D    Subtracts the results of two dice.\n"
						"	D^^D   Rolls on both dice, then selects the higher result.\n"
						"	D__D   Rolls on both dice, then selects the lower result.\n"
						"	D>D    Rolls on both dice, then checks if left is larger than right.\n"
						"	D<D    Rolls on both dice, then checks if left is smaller than right.\n"
						"	D>=D    Rolls on both dice, then checks if left is larger than, or equal to, right.\n"
						"	D<=D    Rolls on both dice, then checks if left is smaller than, or equal to, right.\n"
						"	D=D    Rolls on both dice, then checks if left is equal to right.\n"
						"	D?D    Rolls on the left die and, if the result is less than 1, replaces it with the right die.\n"
						"	D?D:D  Rolls on the leftmost die, and returns the middle die if the result was greater than 0, and the rightmost die otherwise.\n"
						"	(D)    The same as D, used for enforcing operator association.\n"
						" Where n and m represent positive whole numbers, D represents another die, and F is a filter.\n"
						" Operator precedence is as shown.\n"
						"Filters:\n"
						" A filter is a list of signed numbers or ranges such as 'a-b' which selects every value between a and b inclusive.\n"
						" Prefixing a filter with '!' negates it so that every value not listed is rerolled.\n"
						"Patterns:\n"
						" A pattern is either a set, or a die prefixed with a relational operator (<,>,>=,<=,=)\n"
						, argv[0]);
				return EXIT_SUCCESS;

				/** Shorthand for a 1-character long switch that checks for trailing characters */
				#define SWITCH(c,C) case c: case C: if(argv[i][2]) goto bad_arg;

				SWITCH('d','D')
					settings.debug = true;
				continue;

				SWITCH('v','V')
					settings.verbose = !settings.verbose;
				continue;

				SWITCH('q','Q')
					settings.concise = !settings.concise;
				continue;

				case 't':
				case 'T':
					if((argv[i][2] == 'd' || argv[i][2] == 'D') && !argv[i][3])
						settings.dynamicCutoff = true;
					else if((argv[i][2] == 'g' || argv[i][2] == 'G') && !argv[i][3])
						settings.globalCutoff = true;
					else if(argv[i][2])
					{
						char *end;
						settings.cutoff = strtod(&argv[i][2],&end) / 100.0;
						settings.dynamicCutoff = false;

						if(*end)
							goto bad_arg;
					}
					else
					{
						settings.cutoff = 0;
						settings.dynamicCutoff = false;
					}
				continue;

				case 's':
				case 'S':
				{
					int l;
					if(sscanf(&argv[i][2], "%d..%d%n", &settings.rLow, &settings.rHigh, &l) != 2 || argv[i][2 + l])
						goto bad_arg;

					if(settings.rLow > settings.rHigh)
					{
						int h = settings.rLow;
						settings.rLow = settings.rHigh;
						settings.rHigh = h;
					}

					settings.selectRange = true;
				}
				continue;

				case 'o':
				case 'O':
				{
					char *end;
					settings.precision = strtoi(&argv[i][2],&end, 10);

					if(!argv[i][2] || *end)
						goto bad_arg;

					settings.cutoff = pow(10.0, -settings.precision) / 200.0;
				}
				continue;

				case 'w':
				case 'W':
				{
					char *end;
					settings.hcolOverwrite = strtoi(&argv[i][2],&end, 10);

					if(!argv[i][2] || *end)
						goto bad_arg;
				}
				continue;

				case '%':
				{
					char *end;
					settings.percentile = strtoi(&argv[i][2],&end, 10);

					if(!argv[i][2] || *end)
						goto bad_arg;
					if(settings.percentile < 0 || settings.percentile > 100)
					{
						fprintf(stderr, "Bad argument: '%s': %d is not a percentage\n", argv[i], settings.percentile);
						return EXIT_FAILURE;
					}
					if(settings.percentile > 50)
						settings.percentile = 100 - settings.percentile;
				}
				continue;


				case 'r':
				case 'R':
				{
					if(argv[i][2])
					{
						char *end;
						settings.rolls = strtoul(&argv[i][2],&end, 10);

						if(*end || !settings.rolls)
							goto bad_arg;
					}
					else
						settings.rolls = 1;

					settings.mode = ROLL;
				}
				continue;

				SWITCH('p', 'P')
					settings.mode = PREDICT;
				continue;

				case 'c':
				case 'C':
				{
					char *end;
					settings.compareValue = strtoi(&argv[i][2],&end, 10);

					if(!argv[i][2] || *end)
						goto bad_arg;

					settings.mode = COMPARE;
				}
				continue;

				SWITCH('a', 'A')
					settings.mode = PREDICT_COMP;
                    settings.compare = NULL;
				continue;

				SWITCH('n', 'N')
					settings.mode = PREDICT_COMP_NORMAL;
				continue;

				default:
				bad_arg:
					fprintf(stderr, "Bad argument: '%s'\n", argv[i]);
				return EXIT_FAILURE;
			}
		}

		struct die *d = parse(argv[i]);

		if(settings.debug)
			d_printTree(d, 0);

		switch(settings.mode)
		{
			case ROLL:
			{
				int *buf = xcalloc(settings.rolls, sizeof(int));

				for (int i = 0; i < settings.rolls; i++)
					buf[i] = sim(NULL, d);

				printf("%u * ", settings.rolls);
				d_print(d);
				printf(": ");
				prls(buf, settings.rolls);
				putchar('\n');

				free(buf);
			}
			break;

			case PREDICT:
			case PREDICT_COMP:
			case PREDICT_COMP_NORMAL:
			{
				struct prob p = translate(NULL, d);

				d_print(d);
				printf(":\n");

				if(settings.debug)
					p_debug(p);

				if(d_boolean(d))
					p_printB(p);
				else
				{
					double mu, sigma;
					p_header(p, &mu, &sigma);

					if(settings.mode == PREDICT_COMP_NORMAL)
					{
						settings.compare = xmalloc(sizeof(struct prob));
						*settings.compare = (struct prob){ .len = p.len + 2, .low = p.low - 1,
							.p = xmalloc(sizeof(double) * (p.len + 2)) };

						// do this so absolute error is correct & squared error won't be under-reported
						// there is no closed form for the squared error (and the mean doesn't make much sense)
						// so this is the best solution for now
						settings.compare->p[0] = phi(p.low - 0.5, mu, sigma);
						settings.compare->p[p.len + 1] = 1 - phi(p_h(p) + 0.5, mu, sigma);

						for (int i = 0; i < p.len; i++)
							settings.compare->p[i + 1] = normal(mu, sigma, i + p.low);
					}
					if(settings.mode == PREDICT_COMP && settings.compare)
						plot_diff(p, *settings.compare);
					if(!settings.concise)
						p_plot(p);
				}

				if(settings.mode == PREDICT_COMP && !settings.compare)
				{
					settings.compare = xmalloc(sizeof(struct prob));
					settings.compare[0] = p;
				}
				else
					p_free(p);

				if(settings.mode == PREDICT_COMP_NORMAL)
				{
					p_free(*settings.compare);
					free(settings.compare);
					settings.compare = NULL;
				}
			}
			break;

			case COMPARE:
			{
				struct prob p = translate(NULL, d);

				d_print(d);
				printf(" <=> %d:\n", settings.compareValue);
				p_comp(p, settings.compareValue);

				p_free(p);
			}
			break;
		}

		d_freeP(d);
	}

	if(settings.mode == PREDICT_COMP && settings.compare)
	{
		p_free(*settings.compare);
		free(settings.compare);
	}

}