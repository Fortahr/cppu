#pragma once

#include <immintrin.h>
#include <cstdint>
#include <typeindex>

#include "../cgc/pointers.h"
#include "../cgc/constructor.h"
#include "../stor/vector.h"
#include "../hash.h"

#include <unordered_map>
#include <deque>
#include <string_view>
#include <assert.h>
#include <functional>

namespace cppu
{
	namespace serial
	{
		typedef uint16_t Key;
		typedef uint16_t ArchiveVersion;
		typedef uint32_t ValuePos;
		typedef uint32_t ValueSize;
		typedef uint32_t VTableSize;
		typedef uint32_t Reference;

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

		struct VReferenceTableRead
		{
			struct Row
			{
				ValuePos position;
				Reference id;
			};

			ValuePos size;
			Row rows;

			//ValuePos GetRow(Key key) const;
			//void SetRow(Key key, ValuePos pos);
		};

		template <typename Base>
		class base
		{
		public:
			typedef Base Base;

			Base* ptr;

			template <class T>
			base(const T* ptr)
				: ptr(static_cast<Base*>(const_cast<T*>(ptr))) // guarantee T is derived from the base class
			{ }

			template <class T>
			base(T * ptr)
				: ptr(static_cast<Base*>(ptr)) // guarantee T is derived from the base class
			{ }
		};

		class ArchiveWriter;
		class ArchiveReader;
		class SubArchiveWriter;

		struct ArchiveReference
		{
		public:
			Reference key;
			std::function<void()> serialize;

			ArchiveReference(Reference key, std::function<void()> serialize)
				: key(key)
				, serialize(serialize)
			{ }
		};

		class Serializer
		{
			friend class ArchiveReader;
			friend class ArchiveWriter;

		public:
			typedef cppu::hash_t Key;
			typedef std::function<void(ArchiveReader&, void*)> Constructor;

		private:
			static auto& ClassConstructors()
			{
				static std::unordered_map<Key, Constructor> v;
				return v;
			}
		public:
			static auto& ClassIDs()
			{
				static std::unordered_map<std::type_index, cppu::hash_t> v;
				return v;
			}

			static void Register(Key key, Constructor constructor)
			{
				ClassConstructors().try_emplace(key, constructor);
			}

			template<typename T>
			static bool Register();
		};


		class ArchiveWriter
		{
			friend class ArchiveReader;
			friend class SubArchiveWriter;
		private:
			ArchiveVersion version;

			VTableWrite table;
			char* buffer;

			ValuePos bufferSize;
			ValuePos writePosition;

			std::unordered_map<void*, Reference> referencesTaken;
			std::deque<ArchiveReference> references;

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
			bool Serialize(Key key, const T& data);

			bool Serialize(Key key, const void* data, std::size_t size);

			/// <summary>
			/// Serialize containers
			/// </summary>
			/// <typeparam name="It"></typeparam>
			/// <param name="key"></param>
			/// <param name="begin"></param>
			/// <param name="end"></param>
			/// <returns></returns>
			template<typename It>
			bool Serialize(Key key, It begin, It end);

			bool Write(const void* data, std::size_t size);

			template<typename T>
			bool Write(const T& data);

			template<typename It>
			bool Write(It begin, It end);

			template<typename T>
			bool Write(const std::vector<T>& vector);

			template<typename K, typename T>
			bool Write(const std::pair<K, T>& data);

			template<typename... TT>
			bool Write(const std::tuple<TT...>& data);

			template<typename... P>
			void Polymorphic(const P... bases);

			std::string_view Finish();
			void FinishSubArchive(SubArchiveWriter& subArchive);

			template <typename T>
			ArchiveWriter& operator<<(const T& data)
			{
				Write(data);
				return *this;
			}

			SubArchiveWriter CreateSubArchive(VTableSize tableSize, ArchiveVersion version, ValuePos initialBufferSize = 1024);
		};

		class SubArchiveWriter
		{
			friend class ArchiveWriter;
		private:
			ArchiveVersion version;

			ArchiveWriter& writer;
			VTableWrite swappedTable;
			ValuePos startPosition;

			SubArchiveWriter(VTableSize tableSize, ArchiveVersion version, ArchiveWriter& writer, ValuePos startPosition)
				: version(version)
				, writer(writer)
				, startPosition(startPosition)
			{
				swappedTable.rows.resize(tableSize);
			}

		public:
			SubArchiveWriter(SubArchiveWriter&& move)
				: version(std::move(move.version))
				, writer(move.writer)
				, swappedTable(std::move(move.swappedTable))
				, startPosition(std::move(move.startPosition))
			{
			}

			template<typename T>
			bool Serialize(Key key, const T& data);

			bool Serialize(Key key, const void* data, std::size_t size);

			template<typename It>
			bool Serialize(Key key, It begin, It end);

			bool Write(const void* data, std::size_t size);

			template<typename T>
			bool Write(const T& data);

			template<typename It>
			bool Write(It begin, It end);

			template<typename T>
			bool Write(const std::vector<T>& vector);

			template<typename K, typename T>
			bool Write(const std::pair<K, T>& data);

			template<class... TT>
			bool Write(const std::tuple<TT...>& data);

			template<typename... P>
			void Polymorphic(const P... bases);

			template <typename T>
			SubArchiveWriter& operator<<(const T& data)
			{
				writer.Write(data);
				return *this;
			}

			~SubArchiveWriter();
		};

		class ArchiveReader
		{
		public:
			struct Pointer
			{
				bool isSmartPointer;
				byte data[std::max(sizeof(std::shared_ptr<int>), sizeof(cgc::strong_ptr<int>))];

				Pointer(bool isSmartPointer, void* ptr, std::size_t size)
					: isSmartPointer(isSmartPointer)
				{
					memcpy(&data, ptr, size);
				}

				template <typename T>
				T& operator()() { return reinterpret_cast<T&>(data); }
			};

		private:
			bool original;
			char* buffer;
			ValuePos bufferSize;
			ValuePos readPosition;

			VTableRead* table;
			VReferenceTableRead* referenceTable;

			std::unordered_map<Reference, Pointer>* references;

			ArchiveReader(const char* buffer, ValuePos bufferSize, ValuePos readPosition, VTableRead* table, VReferenceTableRead* referenceTable, std::unordered_map<Reference, Pointer>* references)
				: original(false)
				, buffer(const_cast<char*>(buffer))
				, bufferSize(bufferSize)
				, readPosition(readPosition)
				, table(table)
				, referenceTable(referenceTable)
				, references(references)
			{
			}

		public:
			ArchiveReader(const ArchiveWriter& archive)
				: original(true)
			{
				buffer = static_cast<char*>(malloc(archive.writePosition));
				memcpy(buffer, archive.buffer, archive.writePosition);

				bufferSize = reinterpret_cast<ValuePos&>(archive.buffer[0]);
				table = reinterpret_cast<VTableRead*>(buffer + bufferSize);

				if (bufferSize + table->size < archive.writePosition)
				{
					ValuePos offset = bufferSize + sizeof(table->size) + table->size * sizeof(table->rows);
					referenceTable = reinterpret_cast<VReferenceTableRead*>(buffer + reinterpret_cast<ValuePos&>(buffer[offset]));

					references = new std::unordered_map<Reference, Pointer>();
				}
			}

			ArchiveReader(const std::string& string)
				: original(true)
			{
				buffer = static_cast<char*>(malloc(string.size()));
				memcpy(buffer, string.data(), string.size());

				bufferSize = reinterpret_cast<ValuePos&>(buffer[0]);
				table = reinterpret_cast<VTableRead*>(buffer + bufferSize);

				if (bufferSize + table->size < string.size())
				{
					ValuePos offset = bufferSize + sizeof(table->size) + table->size * sizeof(table->rows);
					referenceTable = reinterpret_cast<VReferenceTableRead*>(buffer + reinterpret_cast<ValuePos&>(buffer[offset]));

					references = new std::unordered_map<Reference, Pointer>();
				}
			}

			~ArchiveReader()
			{
				if (original)
					delete references;
			}

			ValuePos GetVTableEntry(Key key);

			void DeSerialize(Key key, void* data, std::size_t size);

			template<typename T>
			void DeSerialize(Key key, T& data);

			template<typename T>
			void DeSerializeContainer(Key key, T& data);

			template<typename T>
			void DeSerialize(Key key, std::list<T>& data);

			template<typename T>
			void ReadPosition(T& data, int position);

			template<typename K, typename T>
			void ReadPosition(std::pair<K, T>& data, int position);

			template<typename... TT>
			void ReadPosition(std::tuple<TT...>& data, int position);

			void ReadPosition(void* data, std::size_t size, int position);

			template<typename T>
			void Read(T& data, int offset = 0);

			void Read(void* data, std::size_t size, int offset = 0);

			template<typename... P>
			void Polymorphic(P... bases);

			ArchiveReader GetSubArchive();
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