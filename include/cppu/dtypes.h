#pragma once

#include <cstdint>

// make sure we got nullptr in our program (nullptr is a c++11 and higher definition)
//#if nullptr 0x0 // TODO: test this on non nullptr assisted compilers
//	typedef decltype(nullptr) nullptr_t;
//#endif

// common used type definitions, the numbers behind the types are not guaranteed
// typedef signed char		char;		// 1 byte		(-127 to 127)
typedef unsigned char		byte;		// 1 byte		(0 to 255) 
// typedef signed short		short;		// 2 bytes		(-32767 to 32767)
typedef unsigned short		ushort;		// 2 bytes		(0 to 65535)
// typedef signed int		int;		// 2/4 bytes	(-32767 to 32767) or (-2147483647 to 2147483647)
typedef unsigned int		uint;		// 2/4 bytes	(0 to 65535) or (0 to 4294967295)
// typedef signed long		long;		// 4 bytes		(-2147483647 to 2147483647)
typedef unsigned long		ulong;		// 4 bytes		(0 to 4294967295)
typedef signed long long	longlong;	// 8 bytes		(-9223372036854775807 to 9223372036854775807)
typedef unsigned long long	ulonglong;	// 8 bytes		(0 to 18446744073709551615)
typedef signed long long	llong;		// 8 bytes		(-9223372036854775807 to 9223372036854775807)
typedef unsigned long long	ullong;		// 8 bytes		(0 to 18446744073709551615)

// (u)int8, (u)int16, (u)int32, (u)int64 type definitions (followed by the C99 minimums)
typedef int8_t				int8;		// 1 byte		(-127 to 127)
typedef int16_t				int16;		// 2 bytes		(-32767 to 32767)
typedef int32_t				int32;		// 4 bytes		(-2147483647 to 2147483647)
typedef int64_t				int64;		// 8 bytes		(-9223372036854775807 to 9223372036854775807)

typedef uint8_t				uint8;		// 1 byte		(0 to 255)
typedef uint16_t			uint16;		// 2 bytes		(0 to 65535)
typedef uint32_t			uint32;		// 4 bytes		(0 to 4294967295)
typedef uint64_t			uint64;		// 8 bytes		(0 to 18446744073709551615)
