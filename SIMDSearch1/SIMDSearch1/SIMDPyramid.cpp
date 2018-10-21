#include "pch.h"
#include "SIMDPyramid.h"


SIMDPyramid::SIMDPyramid(uint32_t* array, uint32_t size) : mainArray(array), mainArraySize(size)
{
	uint32_t lastStackSize = size;
	uint32_t pyramidDepth = 0;
	uint32_t* lastArray = array;

	while (lastStackSize > 32) {
		this->pyramid.push_back(std::make_shared<uint32_t[]>(new uint32_t[lastStackSize / 32]()));

		for (int i = 0; i < lastStackSize/32 ; i++) {
			this->pyramid[pyramidDepth][i] = lastArray[i * 32];
		}

		pyramidDepth++;
		lastArray = pyramid[pyramidDepth - 1].get();
	}
}

SIMDPyramid::~SIMDPyramid()
{
}
