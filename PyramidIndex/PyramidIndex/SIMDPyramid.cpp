#include "stdafx.h"
#include "SIMDPyramid.h"
#include <intrin.h>
#include <windows.h>
#include <random>

// Create 67,108,864 32-bit sorted-ints. 0.25GB space
// Some are repeats
int32_t * millionsOfInts() {
	int32_t* bigArray = (int32_t*)VirtualAlloc(NULL, sizeof(int32_t)*(BENCHMARK_TOTAL_INTS), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	// Use Default seed
	std::mt19937_64 randGenerator;

	// Create an ordered list, but randomly, appropriate for binary_search
	// 0 is ALWAYS in the array
	uint32_t curVal = 0;
	for (unsigned int i = 0; i < BENCHMARK_TOTAL_INTS; i++) {
		bigArray[i] = curVal;

		// Add a random number less than 64 (between 0 and 63).
		curVal += (randGenerator() & (64 - 1)); // 64 is a power of 2, -1 is all the bits smaller than it
	}

	return bigArray;
}

static uint32_t roundUpDivision(uint32_t a, uint32_t b) {
	return ((a - 1) / b) + 1;
}

static void LegacyInt32ArrayDeleter(int32_t* toDelete) {
	delete[](toDelete);
}

SIMDPyramid::SIMDPyramid(const int32_t* const array, uint32_t size) : mainArray(array), mainArraySize(size)
{
	uint32_t lastStackSize = size;
	uint32_t pyramidDepth = 0;
	const int32_t* lastArray = array;

	while (lastStackSize > FACTOR) {
		// The newSize is the actual size of the pyramid.
		// SIMD-friendliness "rounds up" to the next FACTOR, so that 
		// branchless code can search without worry about segfaults.

		// Rounding up division for positive numbers
		uint32_t newSize = roundUpDivision(lastStackSize, FACTOR);
		uint32_t roundedSize = ((newSize / FACTOR) + 1) * FACTOR;

		auto toPushBack = std::shared_ptr<int32_t>(new int32_t[roundedSize], LegacyInt32ArrayDeleter);
		this->pyramid.push_back(toPushBack);
		this->pyramidSizes.push_back(newSize);

		unsigned int i;

		for (i = 0; i < newSize; i++) {
			this->pyramid[pyramidDepth].get()[i] = lastArray[i * FACTOR];
		}

		// Fill up the buffer with "infinity" so that factorSearch works best
		for (; i < roundedSize; i++) {
			this->pyramid[pyramidDepth].get()[i] = INT32_MAX;
		}

		pyramidDepth++;
		lastStackSize = lastStackSize / FACTOR;
		lastArray = pyramid[pyramidDepth - 1].get();
	}
}

// Assume start is aligned. Search exactly "SIMDPyramid::FACTOR" of elements
// Returns the number of elements in the "haystack" that are 
// less-than-or-equal to the needle. (Or: the needle is strictly-greater than haystack)
// Always searches a FACTOR to be more branch-friendly.
uint32_t factorSearch(const int32_t* start, const int32_t needle) {

	__m128i simdNeedle = _mm_set1_epi32(needle);
	uint32_t indexToReturn = 0;
	uint32_t indexToReturn2 = 0; // Manual instruction-level-parallelism

	for (int i = 0; i < SIMDPyramid::FACTOR; i += 8) {
		__m128i* haystack = (__m128i*)&start[i];
		__m128i* haystack2 = (__m128i*)&start[i + 4];
		// Look for a haystack where a value is >= the testValue.
		// "Not greater-than" is also known as "less-than or equal to". 
		// SSE only has "greater-than", so combine with "not" to get the result we want.
		__m128i compareResult = _mm_cmpgt_epi32(simdNeedle, *haystack);
		__m128i compareResult2 = _mm_cmpgt_epi32(simdNeedle, *haystack2);

		indexToReturn += static_cast<uint32_t>(_mm_popcnt_u64(_mm_movemask_epi8(compareResult)) / 4);
		indexToReturn2 += static_cast<uint32_t>(_mm_popcnt_u64(_mm_movemask_epi8(compareResult2)) / 4);
	}

	return indexToReturn + indexToReturn2;
}

uint32_t SIMDPyramid::pyramidSearch(const int32_t needle) {
	// "Index" is the result of the previous pyramid's search.
	uint32_t index = 0;

	for (int i = this->pyramid.size()-1; i >= 0 ; i--) {
		auto next_index = factorSearch(&(this->pyramid[i].get()[index*SIMDPyramid::FACTOR]), needle);
		// There are precisely 3 cases: a "left-side" next_index of 0, a "right side" next_index of >= FACTOR,
		// and a "middle" next_index.

		if (next_index == 0) {
			// If it is not precisely the first element, then the value simply does not exist
			// in the array.
			// In either case, the correct value to return is simply the "current spot", which
			// still needs to be up-converted to the final index.

			// Aside from bigArray[0], I can't think of a unit-test for this case :-(
			// This is correct for bigArray[0] but untested for other values.

			if (this->pyramid[i].get()[index*SIMDPyramid::FACTOR + next_index] == needle) {
				index = index * SIMDPyramid::FACTOR + next_index;
				i--;
				for (; i >= 0; i--) {
					index = index * SIMDPyramid::FACTOR;
				}
				return index * SIMDPyramid::FACTOR;
			}
		}
		else if (index+next_index >= pyramidSizes[i]) {
			// "Right side" case. We must search the next row, as far right as possible
			index = pyramidSizes[i] - 1;
		}
		else {
			// The less-than or equal case can be short-cutted here.
			if (this->pyramid[i].get()[index*SIMDPyramid::FACTOR + next_index] == needle) {
				index = index * SIMDPyramid::FACTOR + next_index;
				i--;
				for (; i >= 0; i--) {
					index = index * SIMDPyramid::FACTOR;
				}
				return index * SIMDPyramid::FACTOR;
			}
			// Otherwise, search down the pyramid
			index = index * SIMDPyramid::FACTOR + next_index - 1;
		}
	}

	// I find it easier to think of the base-case with the FACTOR already
	// in the index.
	index *= SIMDPyramid::FACTOR;

	// Now search the main array. Two cases: we either in the beginning/middle, or we are at the end.
	if (index + SIMDPyramid::FACTOR <= this->mainArraySize) {
		auto next_index = factorSearch(&(this->mainArray[index]), needle);
		return index + next_index;
	}
	else {
		// We don't have SIMDPyramid::FACTOR guarantee. We must carefully search between
		// index and mainArraySize's last section. Just use simple for-loop sequential for clarity
		for (unsigned int  i = index; i < this->mainArraySize; i++) {
			if (this->mainArray[i] == needle) {
				return i;
			}
		}

		return this->mainArraySize;
	}
}

SIMDPyramid::~SIMDPyramid()
{
}
