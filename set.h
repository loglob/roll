// set.h: Implements a set/bag for integers that permits ranges as primitives
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "util.h"


/** A range segment.
	Spans every integer between the given limits, inclusively.
 */
struct range
{
	/** The lower limit */
	signed int start;
	/** The upper limit */
	signed int end;
};

/** A set of integers that permits ranges as primitives.
	Technically a Bag as there is no delete operation. */
struct set
{
	/** An array of range segments, the disjunct union of which forms the overall set. Follows the axioms:
		(0) All segments are ordered such that start <= end
		(1) There are no overlapping ranges
		(2) The ranges are ordered low to high
		(3) Any two adjacent ranges are at least 1 apart, i.e. no two ranges could be merged
	*/
	struct range *entries;
	/** The amount of segments in the entries array. */
	size_t length;
	/** Whether the set is negated */
	bool negated;
};


#define set_has(set, key) set_hasAll(set, key, key)
#define SINGLETON(s,e) (struct set){ (struct range[1]){ { s,e } }, 1, false }


/** Frees a set's contents. */
void set_free(struct set set);

/** Prints a set to stdout.
	Formats it like the argument to \ or ~, without whitespace
 */
void set_print(struct set s);

/** Inserts a range into a set
	@param s The set to operate on
	@param start The start parameter of the set to add
	@param end The end parameter of the set to add
 */
void set_insert(struct set *s, signed int start, signed int end);

/** Determines if a set contains any element in a given range. O(log n)
	@param s The set to test
	@param start The start of the range
	@param end The end of the range
	@returns Whether s contains any element between start and end, inclusive
 */
bool set_hasAny(struct set s, signed int start, signed int end);

/** Determines if a set contains every element in a given range. O(log n)
	@param s The set to test
	@param start The start of the range
	@param end The end of the range
	@returns Whether s contains every element between start and end, inclusive
*/
bool set_hasAll(struct set s, signed int start, signed int end);

/** Determines whether the given set is empty. */
bool set_empty(struct set set);
