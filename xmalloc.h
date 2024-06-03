// xmalloc.h: Succeed-or-die allocation routines to replace libexplain
#pragma once

#define X(f, argsDecl, _1, _2) __attribute_malloc__ void *x##f argsDecl;

#include "xmalloc.c"