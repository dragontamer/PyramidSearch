// PyramidSearchBenchmark.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include "../../PyramidIndex/PyramidIndex/SIMDPyramid.h"
#include "assert.h"
#include "AMDProfileController.h"
#include <random>
#include <algorithm>

void selfTestAsserts(int32_t* theInts) {
	SIMDPyramid p(theInts, BENCHMARK_TOTAL_INTS);
	assert(theInts[p.pyramidSearch(0)] == 0); // Simplest test. Zero is always in array
	assert(theInts[p.pyramidSearch(0x1f82966c)] == 0x1f82966c); // Manually found somewhere in the array
	
	assert(theInts[p.pyramidSearch(0x0417)] == 0x417); // Manually found: location 32. Tests "left-case, mid-pyramid"
	assert(theInts[p.pyramidSearch(0x0416)] == 0x417); // Should still return location 32. Previous element is 405.

	assert(theInts[p.pyramidSearch(0x3f00190a)] == 0x3f00190a); // Manually found: location 1 in top-most pyramid
	assert(theInts[p.pyramidSearch(0x3f001909)] == 0x3f00190a); // Manually found: location 1 in top-most pyramid

	assert(theInts[p.pyramidSearch(0x7dff31d4)] == 0x7dff31d4); // Nearly last value, but doesn't test special case unless I reduce the length of Pyramid

	SIMDPyramid p2(theInts, BENCHMARK_TOTAL_INTS-1);

	assert(theInts[p2.pyramidSearch(0x7dff31d4)] == 0x7dff31d4);  // Nearly the last value, should unit-test the special case at the end.
}

int32_t countBenchmark(int32_t* theInts);

int main()
{
	auto theInts = millionsOfInts();
	int count = 0;

	selfTestAsserts(theInts);

	amdProfileResume();
	SIMDPyramid p(theInts, BENCHMARK_TOTAL_INTS);
	count = countBenchmark(theInts);
	amdProfilePause();

	std::cout << "Count was: " << count << "\n";

	//std::cout << "Count in 10-million case should be: " << 0x000258ff << std::endl;
}