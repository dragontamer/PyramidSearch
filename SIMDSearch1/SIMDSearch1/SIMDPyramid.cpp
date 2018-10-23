#include "pch.h"
#include "SIMDPyramid.h"
#include <intrin.h>

SIMDPyramid::SIMDPyramid(int32_t* array, uint32_t size) : mainArray(array), mainArraySize(size)
{
	uint32_t lastStackSize = size;
	uint32_t pyramidDepth = 0;
	int32_t* lastArray = array;

	while (lastStackSize > FACTOR) {
		// The newSize is the actual size of the pyramid.
		// SIMD-friendliness "rounds up" to the next FACTOR, so that 
		// branchless code can search without worry about segfaults.

		uint32_t newSize = lastStackSize / FACTOR;
		uint32_t roundedSize = ((newSize / FACTOR) + 1) * FACTOR; 

		this->pyramid.push_back(std::make_shared<int32_t[]>(new int32_t[roundedSize]));
		this->pyramidSizes.push_back(newSize);

		int i;

		for (i = 0; i < newSize; i++) {
			this->pyramid[pyramidDepth][i] = lastArray[i * FACTOR];
		}

		// Fill up the buffer with "infinity" so that factorSearch works best
		for (; i < roundedSize; i++) {
			this->pyramid[pyramidDepth][i] = INT32_MAX;
		}

		pyramidDepth++;
		lastArray = pyramid[pyramidDepth - 1].get();
	}
}

// Assume start is aligned. Search exactly "SIMDPyramid::FACTOR" of elements
// Returns the number of elements in the "haystack" that are 
// less-than-or-equal to the needle. (Or: the needle is strictly-greater than haystack)
// Always searches a FACTOR to be more branch-friendly.
uint32_t factorSearch(int32_t* start, int32_t needle) {

	__m128i simdNeedle = _mm_set1_epi32(needle);
	uint32_t indexToReturn = 0;
	uint32_t indexToReturn2 = 0; // Manual instruction-level-parallelism

	for (int i = 0; i < SIMDPyramid::FACTOR; i += 8) {
		__m128i* haystack = (__m128i*)&start[i];
		__m128i* haystack2 = (__m128i*)&start[i+4];
		// Look for a haystack where a value is >= the testValue.
		// "Not greater-than" is also known as "less-than or equal to". 
		// SSE only has "greater-than", so combine with "not" to get the result we want.
		__m128i compareResult = _mm_cmpgt_epi32(simdNeedle, *haystack);
		__m128i compareResult2 = _mm_cmpgt_epi32(simdNeedle, *haystack2);

		indexToReturn += _mm_popcnt_u64(_mm_movemask_epi8(compareResult)) / 4;
		indexToReturn2 += _mm_popcnt_u64(_mm_movemask_epi8(compareResult2)) / 4;
	}

	return indexToReturn+indexToReturn2;
}

bool SIMDPyramid::pyramidSearch(int32_t needle) {
}

// Shared ptrs should clean themselves up. The passed in mainArray* is NOT the responsibility of
// the SIMDPyramid class to clean up.
SIMDPyramid::~SIMDPyramid()
{
}
