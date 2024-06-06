#include "die.h"
#include "parse.h"
#include "pattern.h"
#include <stdio.h>
#include <stdlib.h>


void pt_free(struct Pattern p)
{
	if(p.op)
		d_free(p.die);
	else
		set_free(p.set);
}

void pt_freeD(struct Pattern *p)
{
	if(p)
	{
		pt_free(*p);
		free(p);
	}
}

void pt_print(struct Pattern p)
{
	if(p.op)
	{
		printf("%s ", tkstr(p.op));
		d_print(&p.die);
	}
	else
		set_print(p.set);
}