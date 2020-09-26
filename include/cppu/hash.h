#pragma once

#include <cstdint>
#include <iostream>

namespace cppu
{
	typedef uint32_t hash_t;

	namespace impl
	{
		constexpr const int8_t STRING_END_BYTE = 0;
		constexpr const hash_t STRING_BIT_SHIFT = 5;

		constexpr const uint32_t STRING_HASH_KEY = 5381;

		inline constexpr hash_t hash_function(const char* pTail, hash_t hash = STRING_HASH_KEY)
		{
			return pTail[0] == STRING_END_BYTE
				? hash
				: hash_function(pTail + 1, ((hash << STRING_BIT_SHIFT) + hash) + (int32_t)*pTail);
		}

		inline constexpr hash_t hash_function(const char* pTail, const char* endTail, hash_t hash = STRING_HASH_KEY)
		{
			while (pTail < endTail)
			{
				hash = (hash << STRING_BIT_SHIFT) + hash + static_cast<int32_t>(*pTail);
				pTail++;
			}

			return hash;
		}
			
		template <hash_t hash>
		inline constexpr hash_t compile_time_hash()
		{
			return hash;
		}

	}

	#ifdef _WIN32

	constexpr hash_t RT_HASH(const char* string)
	{
		__pragma(warning(suppress : 4307));
		return impl::hash_function(string);
	}

	constexpr hash_t RT_HASH(const std::string& string)
	{
		__pragma(warning(suppress : 4307));
		return impl::hash_function(string.data(), string.data() + string.size());
	}

	constexpr hash_t RT_HASH(const std::string_view& string)
	{
		__pragma(warning(suppress : 4307));
		return impl::hash_function(string.data(), string.data() + string.size());
	}

	#define CT_HASH(string)  __pragma(warning(suppress : 4307)) \
		impl::compile_time_hash<impl::hash_function(string)>()

	#else
	// TODO: implement these with constexpr and warning ignores for other compilers

	#define RT_HASH(string)  impl::hash_function(string)

	#define RT_STR_HASH(string) impl::hash_function(string.data(), string.data() + string.size())

	#define CT_HASH(string) impl::compile_time_hash<impl::hash_function(string)>()

	#endif
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif