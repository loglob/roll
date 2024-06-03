#include <stdio.h>
#include <stdlib.h>
#include "pattern.h"
#include "parse.h"

void pt_free(struct pattern p)
{
	if(p.op)
		d_free(p.die);
	else
		set_free(p.set);
}

void pt_freeD(struct pattern *p)
{
	if(p)
	{
		pt_free(*p);
		free(p);
	}
}

void pt_print(struct pattern p)
{
	if(p.op)
	{
		printf("%s ", tkstr(p.op));
		d_print(&p.die);
	}
	else
		set_print(p.set);
}