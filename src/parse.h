// parse.h: Defines the interface to the parser
#pragma once

extern const char languageReference[];

/** A human-readable string representation of the given token.
	NOT reentrant.
	Return value is NOT safe after subsequent calls.
 */
const char *tkstr(char tk);

/** Parses a die expression. Exits on parse failure. */
struct Die *parse(const char *str);
