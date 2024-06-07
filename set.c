#include "set.h"
#include "util.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#pragma region Internal Interface

/** Searches for the range segment that contains the given key, if any
	@param n The amount of range entries
	@param entries The range entries
	@param x The key to seek for
	@param lo Overwritten with the highest lower bound, unless NULL 
	@param hi Overwritten with the lowest upper bound, unless NULL 
	@returns A pointer to the range segment containing x, or NULL if there is no such segment
 */
static const struct Range *_set_find(size_t n, const struct Range entries[n], signed int x, const struct Range **lo, const struct Range **hi)
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
static struct Range *_set_merge(struct Set *s, size_t lo, size_t hi)
{
	assert(hi >= lo);

	if(lo != hi)
	{
		s->entries[lo].end = s->entries[hi].end;

		memmove(s->entries + lo + 1, s->entries + hi + 1, sizeof(struct Range) * (s->length - hi - 1));
		s->length -= hi - lo;
		s->entries = xrealloc(s->entries, sizeof(struct Range) * s->length);
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
static struct Range *_set_add(struct Set *s, size_t spl, int start, int end)
{
	s->entries = xrealloc(s->entries, (s->length + 1) * sizeof(struct Range));

	memmove(s->entries + spl + 1, s->entries + spl, (s->length - spl) * sizeof(struct Range));

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
static struct Range *_set_adjust(struct Range *r, signed int start, signed int end)
{
	if(start < r->start)
		r->start = start;
	if(r->end < end)
		r->end = end;

	return r;
}

#pragma endregion


void set_insert(struct Set *s, signed int start, signed int end)
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
		struct Range e = s->entries[i];

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

void set_free(struct Set set)
{
	free(set.entries);
}

bool set_hasAll(struct Set s, signed int start, signed int end)
{
	const struct Range *hit = _set_find(s.length, s.entries, start, NULL, NULL);

	return hit && (end <= hit->end || hit == _set_find(s.length, s.entries, end, NULL, NULL));
}

bool set_hasAny(struct Set s, signed int start, signed int end)
{
	const struct Range *lo = NULL, *hi = NULL;

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

bool set_empty(struct Set set)
{
	return set.length == 0;
}

void set_print(struct Set s)
{
	for (size_t i = 0; i < s.length; i++)
	{
		if(i)
			putchar(',');

		struct Range r = s.entries[i];

		if(r.start == INT_MIN && r.end == INT_MAX)
			putchar('*');
		else if(r.start == INT_MIN)
			printf("*-%d", r.end);
		else if(r.end == INT_MAX)
			printf("%d-*", r.start);
		else if(r.start == r.end)
			printf("%d", r.start);
		else
			printf("%d-%d", r.start, r.end);
	}
}
