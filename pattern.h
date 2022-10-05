// pattern.h: Pattern matching structures & code

/* Patterns are one of:
	rel := '<' | '>' | '>=' | '<=' | '=' | '/=' ;

	P := T
		| F
		| SET
		| rel DIE
	;

	MATCHES := P DIE
		| MATCHES ';' MATCHES
	;

	MATCH := '[' MATCHES ']' ;
*/
#pragma once
#include "die.h"
#include "prob.h"
#include "sim.h"
#include "translate.h"

/** Checks whether a pattern matches an integer. May evaluate dice.
	@param p a pattern
	@param x a value
	@return whether x is matched by p
 */
bool pt_matches(struct pattern p, int x)
{
	if(p.op)
	{
		struct die c = (struct die){ .op = INT, .constant = x };
		struct die d = (struct die){ .op = p.op, .biop = { .l = &c, .r = &p.die } };

		return sim(&d);
	}
	else
		return set_has(p.set, x);
}


/** Determines how likely a probability function will hit a pattern. (!) May return an empty probability function
	@param pt the pattern
	@param p the input probability distribution. Overwritten with the distribution of fallthrough values.
	@returns The overall percentage with which the pattern is hit
 */
double pt_prob(struct pattern pt, struct prob *p)
{
	if(!pt.op)
	{
		double ret = p_has(*p, pt.set);

		if(ret == 1.0)
		{
			p_free(*p);
			*p = (struct prob){};

			return 1.0;
		}
		else
		{
			*p = p_sans(*p, pt.set);

			return ret;
		}
	}
	else
	{
		struct prob q = translate(&pt.die);
		double ret = 0.0;

		switch(pt.op)
		{
			case '>':
			case GT_EQ:
			{
				// current hit probability, i.e. p >(=) q
				double cur = 0.0;

				for (int j = q.low; j < p->low && j <= p_h(q); j++)
					cur += q.p[j - q.low];
				for (int i = 0; i < p->len; i++)
				{
					double peq = probof(q, p->low + i);

					if(pt.op == GT_EQ)
						cur += peq;

					ret += cur * p->p[i];
					p->p[i] *= (1.0 - cur);

					if(pt.op != GT_EQ)
						cur += peq;
				}
			}
			break;

			case '<':
			case LT_EQ:
			{
				// current hit probability, i.e. p <(=) q
				double cur = 1.0;

				for (int j = q.low; j < p->low && j <= p_h(q); j++)
					cur -= q.p[j - q.low];
				for (int i = 0; i < p->len; i++)
				{
					double peq = probof(q, p->low + i);

					if(pt.op != LT_EQ)
						cur -= peq;

					ret += cur * p->p[i];
					p->p[i] *= (1.0 - cur);

					if(pt.op == LT_EQ)
						cur -= peq;
				}
			}
			break;

			case '=':
			case NEQ:
			{
				for (int i = 0; i < p->len; i++)
				{
					double peq = probof(q, p->low + i);

					if(pt.op == NEQ)
						peq = 1.0 - peq;

					ret += peq * p->p[i];
					p->p[i] *= (1.0 - peq);
				}
			}
			break;

			default:
				eprintf("Invalid pattern: Unknown relational operator: %s\n", tkstr(pt.op));
				__builtin_unreachable();
		}

		if(ret == 0.0)
		// pattern never hit
			return 0.0;
		else if(ret == 1.0)
		{
			// pattern always hit
			p_free(*p);
			*p = (struct prob){};
			return 1.0;
		}
		else
		{
			*p = p_cuts(p_scales(*p, 1.0 / (1.0 - ret)), 0, 0);
			return ret;
		}

	}
}

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