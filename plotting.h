#pragma once
#include "prob.h"

/** Prints debug info on p
	@param p a probability function
 */
void p_debug(struct Prob p);

/** Prints a boolean (i.e. 0/1) probability function.
	@param p A probability function
 */
void p_printB(struct Prob p);

/** Prints a header describing a probability function
	@param p A probability function
	@param mu If not NULL, overwritten with the expected value of p
	@param sigma If not NULL, overwritten with the standard deviation of p
 */
void p_header(struct Prob p, double *mu, double *sigma);

/** Plots the difference between two probability functions
	@param p the current probability function
	@param e the expected probability function
 */
void plot_diff(struct Prob p, struct Prob e);

/** Plots p onto stdout.
	@param p A probability function
 */
void p_plot(struct Prob p);

/** Plots p's compare mode against settings.compareValue
	@param p a probability function
	@param to The value to compare against
 */
void p_comp(struct Prob p, int to);
