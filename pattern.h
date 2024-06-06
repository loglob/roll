// pattern.h: AST & functions for pattern matches
#pragma once
#include "ast.h"


void pt_free(struct Pattern pt);
void pt_print(struct Pattern p);
void pt_freeD(struct Pattern *p);