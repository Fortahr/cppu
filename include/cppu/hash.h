#pragma once

#include <cstdint>
#include <iostream>

namespace cppu
{
	typedef uint32_t hash_t;

	namespace details
	{
		constexpr char STRING_END_BYTE = '\0';
		constexpr hash_t STRING_BIT_SHIFT = 5;

		constexpr uint32_t STRING_HASH_KEY = 5381;

		constexpr hash_t hash_function(const char* pTail, hash_t hash = STRING_HASH_KEY) noexcept
		{
			while (pTail[0] != STRING_END_BYTE)
				hash += (hash << STRING_BIT_SHIFT) + static_cast<int32_t>(*pTail++);

			return hash;
		}

		constexpr hash_t hash_function(const char* pTail, const char* endTail, hash_t hash = STRING_HASH_KEY) noexcept
		{
			while (pTail < endTail)
				hash += (hash << STRING_BIT_SHIFT) + static_cast<int32_t>(*pTail++);

			return hash;
		}
	}

	constexpr hash_t hash(const char* string) noexcept
	{
		__pragma(warning(suppress : 4307));
		return details::hash_function(string);
	}

	constexpr hash_t hash(const char* string, const char* end_string) noexcept
	{
		__pragma(warning(suppress : 4307));
		return details::hash_function(string, end_string);
	}

	constexpr hash_t hash(const char* string, size_t size) noexcept
	{
		__pragma(warning(suppress : 4307));
		return details::hash_function(string, string + size);
	}

	constexpr hash_t hash(std::string_view string) noexcept
	{
		__pragma(warning(suppress : 4307));
		return details::hash_function(string.data(), string.data() + string.size());
	}

	constexpr hash_t operator "" _hash(const char* string, size_t size)
	{
		__pragma(warning(suppress : 4307));
		return details::hash_function(string, string + size);
	}

	// may want to move this somewhere else
	template <size_t _Size>
	constexpr size_t strlen(const char(&)[_Size]) noexcept
	{
		return _Size - 1;
	}
	
	constexpr size_t strlen(const char* string)
	{
		size_t size = 0;
		while (*string++ != '\0') ++size;
		return size;
	}
}
