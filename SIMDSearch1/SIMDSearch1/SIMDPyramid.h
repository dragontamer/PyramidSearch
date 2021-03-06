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

	// shared_ptr allows the pyramid arrays to be cloned cheaply (only the pointer is copied)
	std::vector<std::shared_ptr<int32_t[]>> pyramid;
	std::vector<uint32_t> pyramidSizes;

public:
	static const uint32_t FACTOR = 32;

	SIMDPyramid(int32_t* array, uint32_t size);
	bool pyramidSearch(int32_t needle);
	~SIMDPyramid();
};

