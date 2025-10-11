#pragma once
#include <cstdint>
#include <string>

namespace Memory
{
struct PatternData
{
	uint8_t* arrayOfBytes = nullptr;
	char*    mask         = nullptr;
	size_t   length       = 0;

	PatternData() = default;

	~PatternData()
	{
		delete[] arrayOfBytes;
		delete[] mask;
	}

	// disable copy
	PatternData(const PatternData&)            = delete;
	PatternData& operator=(const PatternData&) = delete;

	// enable move
	PatternData(PatternData&& other) noexcept
	{
		arrayOfBytes       = other.arrayOfBytes;
		mask               = other.mask;
		length             = other.length;
		other.arrayOfBytes = nullptr;
		other.mask         = nullptr;
		other.length       = 0;
	}

	PatternData& operator=(PatternData&& other) noexcept
	{
		if (this != &other)
		{
			delete[] arrayOfBytes;
			delete[] mask;

			arrayOfBytes = other.arrayOfBytes;
			mask         = other.mask;
			length       = other.length;

			other.arrayOfBytes = nullptr;
			other.mask         = nullptr;
			other.length       = 0;
		}
		return *this;
	}
};

bool      parseSig(const std::string& sig, PatternData& outPattern);
uintptr_t getBaseAddress();
uintptr_t findPattern(const uint8_t* pattern, const std::string& mask);
} // namespace Memory