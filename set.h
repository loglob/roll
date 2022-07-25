// set.h: Implements a set/bag for integers that permits ranges as primitives
#pragma once
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "xmalloc.h"

/** A range segment. Spans every integer between the given limits, inclusively.*/
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
	/** An array of rang segments, the disjunct union of which forms the overall set. Follows the axioms:
		(0) All segments are ordered such that start <= end
		(1) There are no overlapping ranges
		(2) The ranges are ordered low to high
		(3) Any two adjacent ranges are at least 1 apart, i.e. no two ranges could be merged
	*/
	struct range *entries;
	/** The amount of segments in the entries array. */
	size_t length;
};

#pragma region Internal Interface

/** Searches for the range segment that contains the given key, if any
	@param n The amount of range entries
	@param entries The range entries
	@param x The key to seek for
	@returns A pointer to the range segment containing x, or NULL if there is no such segment
*/
static struct range *_set_find(size_t n, struct range entries[n], signed int x, struct range **lo, struct range **hi)
{
	if(n == 0)
		return NULL;

	size_t m = n / 2;

	if(entries[m].start > x)
	{
		if(hi)
			*hi = entries + m;

		return _set_find(m, entries, x, lo, hi);
	}
	else if(entries[m].end < x)
	{
		if(lo)
			*lo = entries + m;

		return _set_find(n - m - 1, entries + m + 1, x, lo, hi);
	}
	else
		return entries + m;
}


/** Merges multiple ranges in the given set
	@param s The set to operate on
	@param lo The index of the leftmost range
	@param hi THe index of the rightmost range
	@returns A pointer to the merged range segment
 */
static struct range *_set_merge(struct set *s, size_t lo, size_t hi)
{
	assert(hi >= lo);

	if(lo != hi)
	{
		s->entries[lo].end = s->entries[hi].end;

		memmove(s->entries + lo + 1, s->entries + hi + 1, sizeof(struct range) * (s->length - hi - 1));
		s->length -= hi - lo;
		s->entries = xrealloc(s->entries, sizeof(struct range) * s->length);
	}

	return s->entries + lo;
}

/** Adds a new range segment to the given set
	@param s The set to operate on
	@param spl The index to insert the new segment
	@param start The start parameter of the range to add
	@param end The end parameter of the range to add
	@returns A pointer to the new range segment
*/
static struct range *_set_add(struct set *s, size_t spl, int start, int end)
{
	s->entries = xrealloc(s->entries, (s->length + 1) * sizeof(struct range));

	memmove(s->entries + spl + 1, s->entries + spl, (s->length - spl) * sizeof(struct range));

	s->length++;

	s->entries[spl].start = start;
	s->entries[spl].end = end;

	return s->entries + spl;
}

/** Forms a union with the given range segment and (start,end)
	@param r The range segment to edit
	@param start The start parameter of the range
	@param start The end parameter of the range
	@returns The r input pointer
*/
static struct range *_set_adjust(struct range *r, signed int start, signed int end)
{
	if(start < r->start)
		r->start = start;
	if(r->end < end)
		r->end = end;

	return r;
}

#pragma endregion


/** Inserts a range into a set
	@param s The set to operate on
	@param start The start parameter of the set to add
	@param end The end parameter of the set to add
 */
void set_insert(struct set *s, signed int start, signed int end)
{
	// the highest and lowest indices that are fully left/right the given range
	size_t leftHi = leftHi;
	// Whether those indices exist
	bool left = false, right = false;
	// Whether any existing ranges overlap the inserted range, or the range extended by 1 on the left & right
	bool hit = false;
	// The lowest and highest indices that overlap the range
	size_t hitLo = 0, hitHi = 0;

	// do a linear search for simplicity
	for (size_t i = 0; i < s->length; i++)
	{
		struct range e = s->entries[i];

		if(e.end < start - 1)
		{
			left = true;
			leftHi = i;
		}
		if(end + 1 < e.start)
		{
			right = true;
			break;
		}
		if(e.start <= end + 1 && start - 1 <= e.end)
		{
			if(!hit)
				hitLo = i;

			hit = true;
			hitHi = i;
		}
	}

	if(hit)
	// Extend existing ranges to contain the inserted range
		_set_adjust(_set_merge(s, hitLo, hitHi), start, end);
	else if(left && right)
	// Insert a range segment in the middle
		_set_add(s, leftHi + 1, start, end);
	else if(left)
	// every range segment is left of the target, so insert an entry at the end
		_set_add(s, s->length, start, end);
	else
	// no left entries means all segments are right of the target
		_set_add(s, 0, start, end);
}

/** Frees a set's contents. */
void set_free(struct set set)
{
	free(set.entries);
}

/** Determines if a set contains every element in a given range. O(log n)
	@param s The set to test
	@param start The start of the range
	@param end The end of the range
	@returns Whether s contains every element between start and end, inclusive
*/
bool set_hasAll(struct set s, signed int start, signed int end)
{
	struct range *hit = _set_find(s.length, s.entries, start, NULL, NULL);

	return hit && (end <= hit->end || hit == _set_find(s.length, s.entries, end, NULL, NULL));
}

/** Determines if a set contains any element in a given range. O(log n)
	@param s The set to test
	@param start The start of the range
	@param end The end of the range
	@returns Whether s contains any element between start and end, inclusive
 */
bool set_hasAny(struct set s, signed int start, signed int end)
{
	struct range *lo = NULL, *hi = NULL;

	if(_set_find(s.length, s.entries, start, &lo, NULL) || _set_find(s.length, s.entries, end, NULL, &hi))
		return true;

	if(lo && hi)
	// lo <= hi is guaranteed, test if there is at least one entry between the bounds
		return lo + 1 < hi;
	else if(lo)
		// there must be at least one entry right of lo
		return lo < s.entries + s.length - 1;
	else if(hi)
		// there must be at least one entry left of hi
		return hi > s.entries;
	else
		// set is either empty or a subset of the range
		return s.length;
}

#define set_has(set, key) set_hasAll(set, key, key)

#define SINGLETON(s,e) (struct set){ (struct range[1]){ { s,e } }, 1 }

/** Determines whether the given set is empty. */
bool set_empty(struct set set)
{
	return set.length == 0;
}

/** Prints a set to the given file stream. Formats it like the argument to \ or ~, without whitespace */
void set_print(struct set s, FILE *f)
{
	for (size_t i = 0; i < s.length; i++)
	{
		if(i)
			fputc(',', f);

		struct range r = s.entries[i];

		if(r.start == r.end)
			fprintf(f, "%d", r.start);
		else
			fprintf(f, "%d-%d", r.start, r.end);
	}
}