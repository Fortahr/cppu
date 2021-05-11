#pragma once

#include "dtypes.h"
#include <atomic>

#ifdef WIN32
#include <intrin.h>
#endif

/*
 * _BitScanForward https://docs.microsoft.com/nl-nl/cpp/intrinsics/bitscanforward-bitscanforward64
 * Remarks
 * If a set bit is found, the bit position of the first set bit found is returned in the first parameter. If no set bit is found, 0 is returned; otherwise, 1 is returned.
*/

namespace cppu
{
	inline uint32 bsf(uint32 value)
	{
#ifdef WIN32
		unsigned long index = 0;
		unsigned char det = _BitScanForward(&index, value);
		return det ? index : 32;
#elif __GNUC__
		return value == 0 ? 32 : __builtin_ctz(value);
#else
#error "Not implemented"
#endif
	}

	inline uint32 bsf(uint32 value, uint offset)
	{
		value = value >> offset;

#ifdef WIN32
		unsigned long index = 0;
		unsigned char det = _BitScanForward(&index, value);
		return det ? index + offset : 32;
#elif __GNUC__
		return value == 0 ? 32 : __builtin_ctz(value) + offset;
#else
#error "Not implemented"
#endif
	}

	inline uint32 bsr(uint32 value)
	{
#ifdef WIN32
		unsigned long index = 0;
		unsigned char det = _BitScanReverse(&index, value);
		return det ? 31 - index : 32;
#elif __GNUC__
		return value == 0 ? 32 : __builtin_clz(value);
#else
#error "Not implemented"
#endif
	}

	inline uint32 bsr(uint32 value, uint offset)
	{
		value = value << offset;

#ifdef WIN32
		unsigned long index = 0;
		unsigned char det = _BitScanReverse(&index, value);
		return det ? 31 - (index + offset) : 32;
#elif __GNUC__
		return value == 0 ? 32 : __builtin_clz(value) + offset;
#else
#error "Not implemented"
#endif
	}

	// helper functions
	inline uint bs_ltor(uint32 value)
	{
		return bsr(value);
	}

	inline uint bs_ltor(uint32 value, uint offset)
	{
		return bsr(value, offset);
	}

	inline uint bs_rtol(uint32 value)
	{
		return bsf(value);
	}

	inline uint bs_rtol(uint32 value, uint offset)
	{
		return bsf(value, offset);
	}
	
	template<typename T>
	T set_bit(std::atomic<T> &a, T bit)
	{
		T val = a.load();
		T setValue = val | bit;
		
		while (!a.compare_exchange_weak(val, setValue))
			setValue = val | bit;

		return val;
	}
	
	template<typename T>
	T clr_bit(std::atomic<T> &a, T bit)
	{
		T val = a.load();
		T setValue = val & ~bit;
		
		while (!a.compare_exchange_weak(val, setValue))
			setValue = val & ~bit;

		return val;
	}

	template<typename T>
	T set_or_clr_bit(std::atomic<T> &a, T bit, bool set)
	{
		if (set)
			return set_bit<T>(a, bit);
		else
			return clr_bit<T>(a, bit);
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif