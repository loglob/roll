#define _POSIX_C_SOURCE 200809L
//#include "die.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "prob.h"
#include "parse.h"
#include "sim.h"
#include "translate.h"
#include "settings.h"
#include "plotting.h"

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
						"	-v       enables additional printout when using -r\n"
						"	-t[c=0]  Sets the minimum percentage to display in histograms. Specify nothing to disable trimming.\n"
						"	-td      Like -t[c], but sets the minimum value so that it shows at least one histogram dot.\n"
						"	-g       Applies cutoff to every value, not just starting and ending values.\n"
						"	-s[a..b] Shows only the given range of values in histograms.\n"
						"	-o[p]    Sets the output precision for floats. Overwrites -t with the minimum displayable value.\n"
						"	-w[n]    Sets the width of output.\n"
						" Mode arguments:\n"
						"	-r[n=1]  Simulates a dice expression n times.\n"
						"	-p       Prints a histogram for a dice expression.\n"
						"	-ps      Prints a short overview for a dice expression, instead of a full histogram.\n"
						"	-c[v]    Compares a dice expression to a number.\n"
						"	-n       Compares the result percentage to a normal distribution with the same 𝜇 and 𝜎.\n"
						"	-a       Compares the first given die to all following dice.\n"
						"These modes are applied to all following dice, until another mode is specified.\n"
						"The default mode is -p\n"
						"Dice:\n"
						"A die is represented by the language:\n"
						"	n d n      Expands to 'n x d n'.\n"
						"	d n        Rolling a die with n sides.\n"
						"	n          A constant value of n. n may be 0.\n"
						"	D~n,n,...  Rerolls once if any of the given values are rolled. Numbers may be negative.\n"
						"	D\\n,n,...  Functions like ~ with infinite rerolls.\n"
						"	D^n/n      Selects the n highest values out of n tries.\n"
						"	D_n/n      Selects the n lowest values out of n tries.\n"
						"	D^n        Identical to D^1/n.\n"
						"	D_n        Identical to D_1/n.\n"
						"	D!         Rolls D with exploding dice, so another roll of D is added to the maximum value\n"
						"	               and another roll of D is subtracted from the minimum value.\n"
						"	D$n        Like D! but only allows explosions, not implosions, so only maximum rolls are affected.\n"
						"	               Additionally, n specifies how many rounds of explosions are permitted.\n"
						"	D$         Identical to D$1.\n"
						"	DxD        Rolls on the left die, then adds that many rolls of the right die.\n"
						"	D*D        Rolls on both dice, then multiplies the results.\n"
						"	D/D        Rolls on both dice, then divides the results.\n"
						"	D+D        Adds the results of two dice.\n"
						"	D-D        Subtracts the results of two dice.\n"
						"	D^^D       Rolls on both dice, then selects the higher result.\n"
						"	D__D       Rolls on both dice, then selects the lower result.\n"
						"	D>D        Rolls on both dice, then checks if left is larger than right.\n"
						"	D<D        Rolls on both dice, then checks if left is smaller than right.\n"
						"	D?D        Rolls on the left die and, if the result is less than 1, replaces it with the right die.\n"
						"	D?D:D      Rolls on the leftmost die, and returns the middle die if the result was greater than 0, and the rightmost die otherwise.\n"
						"	(D)        The same as D, used for enforcing operator association.\n"
						"Where n represents a positive whole number, and D represents another die.\n"
						"Operator precedence is as shown.\n"
						, argv[0]);
				return EXIT_SUCCESS;

				case 'g':
				case 'G':
				{
					settings.globalCutoff = true;
				}
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

				case 'a':
				case 'A':
					settings.mode = PREDICT_COMP;
				continue;

				case 'n':
				case 'N':
					settings.mode = PREDICT_COMP_NORMAL;
				continue;

				case 'v':
				case 'V':
					if(argv[i][2])
						goto bad_arg;

					settings.verbose = true;
				continue;

				case 'p':
				case 'P':
					if(argv[i][2] == 's' && !argv[i][3])
						settings.concise = true;
					else if(argv[i][2])
						goto bad_arg;
					else
						settings.concise = false;

					settings.mode = PREDICT;
				continue;

				case 'd':
				case 'D':
					settings.debug = true;
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

				case 't':
				case 'T':
					if((argv[i][2] == 'd' || argv[i][2] == 'D') && !argv[i][3])
						settings.dynamicCutoff = true;
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
						settings.rolls= 1;

					settings.mode = ROLL;
				}
				continue;

				default:
				bad_arg:
					fprintf(stderr, "Bad argument: '%s'\n", argv[i]);
				return EXIT_FAILURE;
			}
		}

		struct dieexpr *d = parse(argv[i]);

		if(settings.debug)
			d_printTree(d, 0);

		switch(settings.mode)
		{
			case ROLL:
			{
				int *buf = xcalloc(settings.rolls, sizeof(int));

				for (int i = 0; i < settings.rolls; i++)
					buf[i] = sim(d);

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
				p_t p = translate(d);

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
						settings.compare = xmalloc(sizeof(p_t));
						*settings.compare = (struct prob){ .len = p.len, .low = p.low, .p = xmalloc(sizeof(double) * p.len) };

						for (int i = 0; i < p.len; i++)
							settings.compare->p[i] = normal(mu, sigma, i + p.low);
					}

					if(settings.compare)
						plot_diff(p, *settings.compare);

					if(!settings.verbose)
						p_plot(p);
				}

				if(settings.mode == PREDICT_COMP && !settings.compare)
				{
					settings.compare = xmalloc(sizeof(p_t));
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
				p_t p = translate(d);
				d_print(d);
				printf(" <=> %d:\n", settings.compareValue);
				p_comp(p);
			}
			break;
		}

		d_free(d);
	}

	if(settings.compare)
	{
		p_free(*settings.compare);
		free(settings.compare);
	}

}