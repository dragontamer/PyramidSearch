#pragma once

#include <vector>
#include <memory>

/**
*/

class SIMDPyramid
{
	const uint32_t* const mainArray;
	const uint32_t mainArraySize;

	// SIMDPyramids are read-only. If the original array changes,
	// the SIMDPyramid needs to be rebuilt. Nonetheless, copying the
	// SIMDPyramid seems convenient.

	// shared_ptr allows the pyramid arrays to be cloned cheaply (only the pointer is copied)
	std::vector<std::shared_ptr<uint32_t[]>> pyramid;

public:
	SIMDPyramid(uint32_t* array, uint32_t size);
	~SIMDPyramid();
};

