#pragma once

#include <immintrin.h>
#include <cstdint>
#include "../stor/vector.h"

#include <unordered_map>
#include <string_view>
#include <assert.h>

namespace cppu
{
	namespace serial
	{
		typedef uint16_t Key;
		typedef uint16_t ArchiveVersion;
		typedef uint32_t ValuePos;
		typedef uint32_t VTableSize;
		typedef uint32_t SerializedSize;

		struct VTableWrite
		{
			cppu::stor::vector<ValuePos, Key> rows;

			ValuePos GetRow(Key key) const;
			void SetRow(Key key, ValuePos pos);
		};

		struct VTableRead
		{
			ValuePos size;
			ValuePos rows;

			ValuePos GetRow(Key key) const;
			void SetRow(Key key, ValuePos pos);
		};

		class ArchiveWriter;
		class ArchiveReader;

		/*class Serializer
		{
			static std::unordered_map<Key, void(ArchiveReader&)>& GetClassTable()
			{
				static std::unordered_map<Key, void(ArchiveReader&)> v;
				return v;
			}
		};*/

		class ArchiveWriter
		{
			friend class ArchiveReader;
		private:
			ArchiveVersion version;

			VTableWrite table;
			//cppu::stor::vector<char, ValuePos> buffer;
			char* buffer;

			ValuePos bufferSize;
			ValuePos writePosition;

			bool Reserve(ValuePos size);
			bool EnoughCapacityOrReserve(ValuePos size);

			void AddKeyAndWrite(Key key, const void* data, std::size_t size);
			void AddKeyAndWriteDirect(Key key, const void* data, std::size_t size);
			void WriteDirect(const void* data, std::size_t size);

			VTableWrite& GetVTable();
			void AddVTableEntry(Key key);

		public:
			ArchiveWriter(VTableSize tableSize, ArchiveVersion version, ValuePos initialBufferSize = 1024)
				: version(version)
				, writePosition(sizeof(ValuePos) + sizeof(ArchiveVersion))
				, bufferSize(initialBufferSize)
			{
				table.rows.resize(tableSize);
				buffer = static_cast<char*>(malloc(initialBufferSize + sizeof(ValuePos)));

#if defined(DEBUG) || defined(_DEBUG)
				memset(buffer, 0, sizeof(ValuePos)); // can be used to check if Finish() has already been called
#endif
				memcpy(buffer + sizeof(ValuePos), &version, sizeof(ArchiveVersion));

			}

			~ArchiveWriter()
			{
				free(buffer);
			}

			template<typename T>
			void Serialize(Key key, const T& data);

			void Serialize(Key key, const void* data, std::size_t size);

			template<typename It>
			void Serialize(Key key, It begin, It end);

			void Write(const void* data, std::size_t size);

			template<typename T>
			void Write(const T& data);

			std::string_view Finish();
		};

		class ArchiveReader
		{
		private:
			ValuePos pointer;

			char* buffer;
			ValuePos bufferSize;

			VTableRead* table;

		public:
			ArchiveReader(const ArchiveWriter& archive)
			{
				buffer = static_cast<char*>(malloc(archive.writePosition));
				memcpy(buffer, archive.buffer, archive.writePosition);

				bufferSize = reinterpret_cast<ValuePos&>(archive.buffer[0]);
				table = reinterpret_cast<VTableRead*>(buffer + bufferSize);
			}

			ValuePos GetVTableEntry(Key key);

			void DeSerialize(Key key, void* data, std::size_t size);

			template<typename T>
			void DeSerialize(Key key, T& data);
		};
	}
}

// The wire format uses a little endian encoding (since that's efficient for
// the common platforms).
#if defined(__s390x__)
#	define SERIALIZER_LITTLE_ENDIAN 0
#endif // __s390x__
#	if !defined(SERIALIZER_LITTLE_ENDIAN)
#		if defined(__GNUC__) || defined(__clang__) || defined(__ICCARM__)
#			if (defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
#				define SERIALIZER_LITTLE_ENDIAN 0
#			else
#				define SERIALIZER_LITTLE_ENDIAN 1
#			endif // __BIG_ENDIAN__
#		elif defined(_MSC_VER)
#			if defined(_M_PPC)
#				define SERIALIZER_LITTLE_ENDIAN 0
#			else
#				define SERIALIZER_LITTLE_ENDIAN 1
#			endif
#	else
#		error Can't figure out platform's endiannes, please define SERIALIZER_LITTLE_ENDIAN.
#	endif
#endif // !defined(SERIALIZER_LITTLE_ENDIAN)

#define SERIALIZER_SWAP_ENDIAN SERIALIZER_LITTLE_ENDIAN == 0

namespace cppu
{
	namespace serial
	{
		namespace details
		{
			template<typename T>
			inline T SwapEndian(T value)
			{
				// code thanks to Google FlatBuffers
#				if defined(_MSC_VER)
#					define PLATFORM_BYTE_SWAP_16 _byteswap_ushort
#					define PLATFORM_BYTE_SWAP_32 _byteswap_ulong
#					define PLATFORM_BYTE_SWAP_64 _byteswap_uint64
#				elif defined(__ICCARM__)
#					define PLATFORM_BYTE_SWAP_16 __REV16
#					define PLATFORM_BYTE_SWAP_32 __REV
#					define PLATFORM_BYTE_SWAP_64(x) \
						((__REV(static_cast<uint32_t>(x >> 32U))) | (static_cast<uint64_t>(__REV(static_cast<uint32_t>(x)))) << 32U)
#				else
#					if defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ < 408 && !defined(__clang__)
						// __builtin_bswap16 was missing prior to GCC 4.8.
#						define PLATFORM_BYTE_SWAP_16(x) static_cast<uint16_t>(__builtin_bswap32(static_cast<uint32_t>(x) << 16))
#					else
#						define PLATFORM_BYTE_SWAP_16 __builtin_bswap16
#					endif
#					define PLATFORM_BYTE_SWAP_32 __builtin_bswap32
#					define PLATFORM_BYTE_SWAP_64 __builtin_bswap64
#				endif
				if constexpr (sizeof(T) == 1)   // Compile-time if-then's.
					return value;
				else if constexpr (sizeof(T) == 2)
					reinterpret_cast<uint16_t&>(value) = PLATFORM_BYTE_SWAP_16(reinterpret_cast<uint16_t&>(value));
				else if constexpr (sizeof(T) == 4)
					reinterpret_cast<uint32_t&>(value) = PLATFORM_BYTE_SWAP_32(reinterpret_cast<uint32_t&>(value));
				else if constexpr (sizeof(T) == 8)
					reinterpret_cast<uint64_t&>(value) = PLATFORM_BYTE_SWAP_64(reinterpret_cast<uint64_t&>(value));
				//else
				//	static_assert(0);

				return value;
			}

			template<typename T>
			inline T CorrectEndian(T& value)
			{
#			if SERIALIZER_LITTLE_ENDIAN == 1
				return value;
#			else
				return SwapEndian(value);
#			endif
			}
		}
	}
}

#include "./Serializer.inl"