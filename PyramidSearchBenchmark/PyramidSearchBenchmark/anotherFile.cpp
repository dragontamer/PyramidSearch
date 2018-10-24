#include "pch.h"
#include <stdint.h>
#include <random>
#include "../../PyramidIndex/PyramidIndex/SIMDPyramid.h"
#include <algorithm>

/**
AMD's uProf API seems to have glitched out "PyramidSearchBenchmark.cpp". Anything that should be 
tracked inside of the uProf GUI should go here instead.
*/

bool pyramidSearch(SIMDPyramid& p, int32_t* theInts, uint32_t needle) {
	uint32_t result = p.pyramidSearch(needle);
	if (result < BENCHMARK_TOTAL_INTS && theInts[result] == needle) {
		return true;
	}
	return false;
}

bool binarySearch(int32_t* theInts, uint32_t needle) {
	return std::binary_search(theInts, theInts + BENCHMARK_TOTAL_INTS, needle);
}

int32_t pyramidSearch10Million(int32_t* theInts) {
	std::mt19937_64 randGenerator;
	int32_t count = 0;
	SIMDPyramid p(theInts, BENCHMARK_TOTAL_INTS);
	for (int i = 0; i < 10000000; i++) {
		uint32_t needle = static_cast<uint32_t>(randGenerator());
		if (pyramidSearch(p, theInts, needle)) {
			count++;
		}
	}
	return count;
}

int32_t binarySearch10Million(int32_t* theInts) {
	std::mt19937_64 randGenerator;
	int32_t count = 0;
	for (int i = 0; i < 10000000; i++) {
		uint32_t needle = static_cast<uint32_t>(randGenerator());
		if (binarySearch(theInts, needle)) {
			count++;
		}
	}
	return count;
}

int32_t pyramidSearch100k(int32_t* theInts) {
	std::mt19937_64 randGenerator;
	int32_t count = 0;
	SIMDPyramid p(theInts, BENCHMARK_TOTAL_INTS);
	for (int i = 0; i < 100000; i++) {
		uint32_t needle = static_cast<uint32_t>(randGenerator());
		if (pyramidSearch(p, theInts, needle)) {
			count++;
		}
	}
	return count;
}

int32_t binarySearch100k(int32_t* theInts) {
	std::mt19937_64 randGenerator;
	int32_t count = 0;
	for (int i = 0; i < 100000; i++) {
		uint32_t needle = static_cast<uint32_t>(randGenerator());
		if (binarySearch(theInts, needle)) {
			count++;
		}
	}
	return count;
}

int32_t pyramidSearch1k(int32_t* theInts) {
	std::mt19937_64 randGenerator;
	int32_t count = 0;
	SIMDPyramid p(theInts, BENCHMARK_TOTAL_INTS);

	for (int i = 0; i < 1000; i++) {
		uint32_t needle = static_cast<uint32_t>(randGenerator());
		if (pyramidSearch(p, theInts, needle)) {
			count++;
		}
	}
	return count;
}

int32_t binarySearch1k(int32_t* theInts) {
	std::mt19937_64 randGenerator;
	int32_t count = 0;
	for (int i = 0; i < 1000; i++) {
		uint32_t needle = static_cast<uint32_t>(randGenerator());
		if (binarySearch(theInts, needle)) {
			count++;
		}
	}
	return count;
}


int32_t countBenchmark(int32_t* theInts) {
	// Keeping the total count will prevent any overly aggressive
	// optimizer from removing the code.
	int32_t totalCount = 0; 

	totalCount += pyramidSearch10Million(theInts);
	totalCount += binarySearch10Million(theInts);
	//totalCount += pyramidSearch100k(theInts);
	//totalCount += binarySearch100k(theInts);
	//totalCount += pyramidSearch1k(theInts);
	//totalCount += binarySearch1k(theInts);

	return totalCount;
}