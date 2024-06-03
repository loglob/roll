// sim.h: Implements simulating die rolls
#pragma once
#include "die.h"

/** Simulates a die roll.
	If settings.verbose is true outputs additional information for each roll, such as intermediate results.
	@param p A die expression
	@returns The result of rolling d
*/
int sim(struct die *d);
