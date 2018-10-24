#pragma once

#include <vector>
#include <memory>

/**
Prototype#2, working on 32-bit integers still but I'm isolating the functions to its own class.
*/

class SIMDPyramid
{
	const int32_t* const mainArray;
	const uint32_t mainArraySize;

	// SIMDPyramids are read-only. If the original array changes,
	// the SIMDPyramid needs to be rebuilt. Nonetheless, copying the
	// SIMDPyramid seems convenient.

	// Be sure to erase properly with delete []
	std::vector<std::shared_ptr<int32_t>> pyramid;
	std::vector<uint32_t> pyramidSizes;

public:
	static constexpr uint32_t FACTOR = 32;

	SIMDPyramid(const int32_t* const array, const uint32_t size);

	// Returns the index
	uint32_t pyramidSearch(const int32_t needle);
	~SIMDPyramid();
};

// For Benchmark Purposes
constexpr uint32_t BENCHMARK_TOTAL_INTS = 1 << 26;
int32_t * millionsOfInts();