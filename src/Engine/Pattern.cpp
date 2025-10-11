#include "pch.hpp"
#include "Pattern.hpp"

namespace Memory
{
// Parses a pretty pattern string into an AOB + mask
bool parseSig(const std::string& sig, PatternData& outPattern)
{
	size_t   maxLen   = sig.size() / 2 + 1;
	uint8_t* tmpBytes = new uint8_t[maxLen];
	char*    tmpMask  = new char[maxLen + 1];

	size_t  byteIndex   = 0;
	int     nibbleCount = 0;
	uint8_t currentByte = 0;

	auto hexToNibble = [](char c) -> int
	{
		if (c >= '0' && c <= '9')
			return c - '0';
		if (c >= 'A' && c <= 'F')
			return c - 'A' + 10;
		if (c >= 'a' && c <= 'f')
			return c - 'a' + 10;
		return -1;
	};

	for (size_t i = 0; i < sig.size(); ++i)
	{
		char c = sig[i];

		if (std::isspace(static_cast<unsigned char>(c)))
			continue;

		if (c == '?')
		{
			if (nibbleCount == 1)
			{
				tmpBytes[byteIndex]  = 0x00;
				tmpMask[byteIndex++] = '?';
				nibbleCount          = 0;
			}
			else
			{
				if (i + 1 < sig.size() && sig[i + 1] == '?')
					++i;
				tmpBytes[byteIndex]  = 0x00;
				tmpMask[byteIndex++] = '?';
			}
			continue;
		}

		int nibble = hexToNibble(c);
		if (nibble == -1)
		{
			delete[] tmpBytes;
			delete[] tmpMask;
			return false;
		}

		if (nibbleCount == 0)
		{
			currentByte = nibble << 4;
			nibbleCount = 1;
		}
		else
		{
			currentByte |= nibble;
			tmpBytes[byteIndex]  = currentByte;
			tmpMask[byteIndex++] = 'x';
			nibbleCount          = 0;
		}
	}

	if (nibbleCount == 1)
	{
		tmpBytes[byteIndex]  = 0x00;
		tmpMask[byteIndex++] = '?';
	}

	delete[] outPattern.arrayOfBytes;
	delete[] outPattern.mask;

	outPattern.arrayOfBytes = new uint8_t[byteIndex];
	outPattern.mask         = new char[byteIndex + 1];
	outPattern.length       = byteIndex;

	std::memcpy(outPattern.arrayOfBytes, tmpBytes, byteIndex);
	std::memcpy(outPattern.mask, tmpMask, byteIndex);
	outPattern.mask[byteIndex] = '\0';

	delete[] tmpBytes;
	delete[] tmpMask;
	return true;
}

uintptr_t findPattern(const uint8_t* pattern, const std::string& mask)
{
	if (pattern && !mask.empty())
	{
		MODULEINFO moduleInfo;
		ZeroMemory(&moduleInfo, sizeof(MODULEINFO));

		HMODULE hModule = GetModuleHandle(NULL);
		GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(MODULEINFO));

		uintptr_t start = reinterpret_cast<uintptr_t>(hModule);
		uintptr_t end   = (start + moduleInfo.SizeOfImage);

		size_t currentPos = 0;
		size_t maskLength = (mask.length() - 1);

		for (uintptr_t retAddress = start; retAddress < end; retAddress++)
		{
			if (*reinterpret_cast<uint8_t*>(retAddress) == pattern[currentPos] || mask[currentPos] == '?')
			{
				if (currentPos == maskLength)
					return (retAddress - maskLength);

				currentPos++;
			}
			else
			{
				retAddress -= currentPos;
				currentPos = 0;
			}
		}
	}

	return NULL;
}
uintptr_t getBaseAddress()
{
	static uintptr_t baseAddr = reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));
	return baseAddr;
}

uintptr_t getOffset(void* pointer)
{
	static uintptr_t baseAddress = getBaseAddress();

	uintptr_t address = reinterpret_cast<uintptr_t>(pointer);

	if (address > baseAddress)
		return (address - baseAddress);

	return NULL;
}
} // namespace Memory