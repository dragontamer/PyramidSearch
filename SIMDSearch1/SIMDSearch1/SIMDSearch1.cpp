// SIMDSearch1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <random>
#include <Windows.h>
#include <algorithm>
#include <assert.h>
#include <intrin.h>

const uint32_t TOTAL_INTS = 1 << 26; 

// Smaller, faster function for debugging purposes.
// 1024 ints only
uint32_t * thousandsOfInts() {
	uint32_t* bigArray = (uint32_t*)VirtualAlloc(NULL, sizeof(int32_t)*(1024), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	// Use Default seed
	std::mt19937_64 randGenerator;

	// Create an ordered list, but randomly, appropriate for binary_search
	// 0 is ALWAYS in the array
	uint32_t curVal = 0;
	for (unsigned int i = 0; i < 1024; i++) {
		bigArray[i] = curVal;

		// Add a random number less than 64 (between 0 and 63).
		curVal += (randGenerator() & (64 - 1)); // 64 is a power of 2, -1 is all the bits smaller than it
	}

	return bigArray;
}

// Create 67,108,864 32-bit sorted-ints. 0.25GB space
// Some are repeats
uint32_t * millionsOfInts() {
	uint32_t* bigArray = (uint32_t*) VirtualAlloc(NULL, sizeof(int32_t)*(TOTAL_INTS), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	// Use Default seed
	std::mt19937_64 randGenerator;

	// Create an ordered list, but randomly, appropriate for binary_search
	// 0 is ALWAYS in the array
	uint32_t curVal = 0;
	for (unsigned int i = 0; i < TOTAL_INTS; i++) {
		bigArray[i] = curVal;

		// Add a random number less than 64 (between 0 and 63).
		curVal += (randGenerator() & (64-1)); // 64 is a power of 2, -1 is all the bits smaller than it
	}

	return bigArray;
}

//uint32_t countArray[0x000258ff];

uint64_t time10MillionBinarySearch(uint32_t* bigSortedArray) {
	LARGE_INTEGER start, end;
	int count = 0;

	// Default Seed
	std::mt19937_64 randGenerator;

	QueryPerformanceCounter(&start);
	for (unsigned int i = 0; i < 10000000; i++) {
		uint32_t needle = static_cast<uint32_t>(randGenerator());
		if (std::binary_search(bigSortedArray, bigSortedArray + TOTAL_INTS, needle)) {
			count++; // Count the number of hits between 0 and 10-million
		}
	}
	QueryPerformanceCounter(&end);

	// Magic number experimentally found.
	// I assume std::binary_search to be implemented correctly
	assert(count == 0x000258ff);

	return end.QuadPart - start.QuadPart;
}

// Assume start and end are SIMD-aligned, 128-bit aligned.
// Returns index from start where SIMD stopped, equivalent to "lower bound" in std::lower_bound
// Looking for a needle in the haystack
int simd_sequential_search(uint32_t* start, uint32_t* end, uint32_t needle) {
	__m128i simdNeedle = _mm_set1_epi32(needle);

	__m128i allOnes = _mm_set1_epi64x(-1);
	__m128i* simdStart = reinterpret_cast<__m128i*> (start);
	__m128i* simdEnd = reinterpret_cast<__m128i*> (end);
	__m128i* haystack;
	__m128i compareResult = _mm_setzero_si128();

	for (haystack = simdStart; haystack < simdEnd; haystack++) {
		// Look for a haystack where a value is >= the testValue.
		// "Not greater-than" is also known as "less-than or equal to". 
		// SSE only has "greater-than", so combine with "not" to get the result we want.
		compareResult = _mm_cmpgt_epi32(simdNeedle, *haystack);
		__m128i lessThanEqualToResults = _mm_andnot_si128(compareResult, allOnes); 

		//Break if ANY haystack >= needle (if the results are non-zero)
		//testz is a zero-test.
		if (!_mm_testz_si128(lessThanEqualToResults, lessThanEqualToResults)) {
			break;
		}
	}

	return (haystack - simdStart) * 4  // Number of SIMD-chunks of 4 that we searched through completely
		+ __popcnt(_mm_movemask_epi8(compareResult)) / 4; // Number within SIMD-chunk, between [0 and 4]
	// movemask operates on a 8-bit level, returning 16-bits
	// The popcount results in 0 to 16, divide by 4 to result in a value between 0 and 4
}

uint64_t  time10MillionSIMDPyramidSearch(uint32_t* bigSortedArray) {
	LARGE_INTEGER start, end;
	int count = 0;

	// Default Seed
	std::mt19937_64 randGenerator;

	// I'm including the cost of building and allocating the SIMDPyramid
	QueryPerformanceCounter(&start);

	// The data-structure! Kind of raw, but enough for a proof of concept
	uint32_t* pyramid[4];
	uint32_t pyramidSizes[4];

	pyramidSizes[0] = TOTAL_INTS / 64;
	for (int i = 1; i < 4; i++) {
		pyramidSizes[i] = pyramidSizes[i - 1] / 64;
	}

	for (int i = 0; i < 4; i++) {
		pyramid[i] = (uint32_t*)VirtualAlloc(NULL, sizeof(int32_t)*pyramidSizes[i], MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	}

	// Base case: pyramid[0] is 1/64th the bigArray
	for (uint32_t i = 0; i < pyramidSizes[0]; i++){
		pyramid[0][i] = bigSortedArray[i * 64];
	}

	// Recursive case: pyramid[i] is 1/64th the pyramid[i-1]
	for (int i = 1; i < 4; i++) {
		for (uint32_t j = 1; j < pyramidSizes[i]; j++) {
			pyramid[i][j] = pyramid[i - 1][j * 64];
		}
	}

	for (unsigned int i = 0; i < 10000000; i++) {

		// SIMDPyramid search algorithm.
		// Search the tallest pyramid with a SIMD-friendly sequential search
		// Use information to figure out where in the next level of the pyramid to search for.
		// Ex: If the element is between #30 and #31, then it will be between 30*64 and 31*64 in the next level.
		// This ensures a worst-case sequential-search of 64 at every level, which is more cache friendly,
		// prefetcher-friendly, and SIMD friendly than binary search.

		uint32_t needle = static_cast<uint32_t>(randGenerator());

		uint32_t index= simd_sequential_search(pyramid[3], pyramid[3] + pyramidSizes[3], needle);
		if (index >= pyramidSizes[3]) {
			index = pyramidSizes[3]-1;
		} else if (index == 0) {
			// we either equal the index of 0, or it isn't in the array
			if (pyramid[3][0] == needle)
				goto foundIt;
			goto notFoundIt;
		}
		else {
			if (pyramid[3][index] == needle)
				goto foundIt;
			index--;
		}

		uint32_t next_index;

		for (int j = 2; j >= 0; j--) {
			// Note: this is going to be relative to (index*64). Compensate by adding (index*64) to the result later.
			next_index = simd_sequential_search(pyramid[j] + (index) * 64, pyramid[j] + (index+1) * 64, needle);
			if (next_index >= 64) {
				next_index = 63;
			}
			else if (next_index == 0) {
				// we either equal the index of 0, or it isn't in the array
				if (pyramid[j][0] == needle)
					goto foundIt; 
				goto notFoundIt; 
			}
			else {
				if (pyramid[j][index*64 + next_index] == needle)
					goto foundIt;
				next_index--;
			}

			index = index * 64 + next_index;
		}
		next_index = simd_sequential_search(bigSortedArray + (index) * 64, bigSortedArray + (index+1) * 64, needle);
		index = (index * 64) + next_index;
		if (index < TOTAL_INTS && bigSortedArray[index] == needle) {
			foundIt: 
			//assert(countArray[count] == needle);
			count++;
		}
		notFoundIt:
		index = 0;
	}
	QueryPerformanceCounter(&end);

	assert(count == 0x000258ff);

	return end.QuadPart - start.QuadPart;
}

int main()
{
	LARGE_INTEGER frequency;

	QueryPerformanceFrequency(&frequency);

	uint32_t* bigSortedArray = millionsOfInts();

	std::cout << "Beginning 10 Million Binary Searches" << std::endl; 

	uint64_t binarySearchTime = time10MillionBinarySearch(bigSortedArray);

	std::cout << "Binary Search Time (uS): " << binarySearchTime * 1000000 / frequency.QuadPart << std::endl;

	uint64_t pyramidSearchTime = time10MillionSIMDPyramidSearch(bigSortedArray);

	std::cout << "SIMDPyramid Search Time (uS): " << pyramidSearchTime * 1000000 / frequency.QuadPart << std::endl;


	/*uint32_t* intArray = thousandsOfInts();

	std::cout << simd_sequential_search(intArray, intArray + 1024, 0) << std::endl;
	std::cout << simd_sequential_search(intArray, intArray + 1024, 0x26) << std::endl;
	std::cout << simd_sequential_search(intArray, intArray + 1024, 0x1158) << std::endl;
	std::cout << simd_sequential_search(intArray, intArray + 1024, 4450) << std::endl;

	std::cout << "Reprinting: " << intArray[simd_sequential_search(intArray, intArray + 1024, 0)] << std::endl;
	std::cout << "Reprinting: " << intArray[simd_sequential_search(intArray, intArray + 1024, 0x26)] << std::endl;
	std::cout << "Reprinting: " << intArray[simd_sequential_search(intArray, intArray + 1024, 0x1158)] << std::endl;
	std::cout << "Reprinting: " << intArray[simd_sequential_search(intArray, intArray + 1024, 4450)] << std::endl*/;

	//std::cout << "Reprinting: " << bigSortedArray[simd_sequential_search(bigSortedArray, bigSortedArray + TOTAL_INTS, 0x56AA413)] << std::endl;

}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
