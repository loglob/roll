// pattern.h: AST & functions for pattern matches
#pragma once
#include "ast.h"


void pt_free(struct pattern pt);
void pt_print(struct pattern p);
void pt_freeD(struct pattern *p);