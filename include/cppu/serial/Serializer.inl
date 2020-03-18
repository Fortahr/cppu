#pragma once

#include "./Serializer.h"

#include <type_traits>

namespace cppu
{
	namespace serial
	{
		inline ValuePos VTableWrite::GetRow(Key key) const
		{
			assert(key < rows.size());
			return rows[key];
		}

		inline void VTableWrite::SetRow(Key key, ValuePos pos)
		{
			assert(key < rows.size());
			rows[key] = pos;
		}

		inline ValuePos VTableRead::GetRow(Key key) const
		{
			assert(key < size);
			return (&rows)[key];
		}

		inline void VTableRead::SetRow(Key key, ValuePos pos)
		{
			assert(key < size);
			(&rows)[key] = pos;
		}

		inline std::string_view ArchiveWriter::Finish()
		{
			assert(*reinterpret_cast<ValuePos*>(buffer) == 0);

			if (EnoughCapacityOrReserve(sizeof(ValuePos) + table.rows.size() * sizeof(ValuePos)))
			{
				memcpy(buffer, &writePosition, sizeof(writePosition));

				ValuePos vtableCount = table.rows.size();
				ValuePos vtableSize = table.rows.size() * sizeof(ValuePos);

				memcpy(buffer + writePosition, &vtableCount, sizeof(ValuePos));
				writePosition += sizeof(ValuePos);
				memcpy(buffer + writePosition, table.rows.data(), vtableSize);
				writePosition += vtableSize;
			}
			// ELSE:

			return std::string_view(buffer, writePosition);
		}

		
		inline bool ArchiveWriter::Reserve(ValuePos size)
		{
			char* newBuffer = static_cast<char*>(realloc(buffer, size));

			if (newBuffer == nullptr)
			{
				// something went wrong, error!
				free(buffer);
				return false;
			}
			else if (newBuffer != buffer)
				buffer = newBuffer;
			
			bufferSize = size;

			return true;
		}

		inline bool ArchiveWriter::EnoughCapacityOrReserve(ValuePos size)
		{
			return writePosition + size <= bufferSize || Reserve(size);
		}

		inline void ArchiveWriter::AddVTableEntry(Key key)
		{
			table.SetRow(key, static_cast<ValuePos>(writePosition));
		}

		inline void ArchiveWriter::AddKeyAndWrite(Key key, const void* data, std::size_t size)
		{
			if (EnoughCapacityOrReserve(size))
				AddKeyAndWriteDirect(key, data, size);
		}

		inline void ArchiveWriter::AddKeyAndWriteDirect(Key key, const void* data, std::size_t size)
		{
			AddVTableEntry(key);
			WriteDirect(data, size);
		}

		inline void ArchiveWriter::WriteDirect(const void* data, std::size_t size)
		{
			memcpy(buffer + writePosition, data, size);
			writePosition += size;
		}

		inline void ArchiveWriter::Write(const void* data, std::size_t size)
		{
			if (EnoughCapacityOrReserve(size))
				WriteDirect(&data, size);
			// ELSE:
		}

		template<typename T>
		inline void ArchiveWriter::Write(const T& data)
		{
			WriteDirect(reinterpret_cast<const char*>(&data), sizeof(T));
		}

		inline void ArchiveWriter::Serialize(Key key, const void* data, std::size_t size)
		{
			if (EnoughCapacityOrReserve(size))
			{
				AddVTableEntry(key);
				WriteDirect(&data, size);
			}
			// ELSE:
		}

		template <typename T, typename = ArchiveWriter&> struct ClassHasSerialize : std::false_type { };
		template <typename T> struct ClassHasSerialize<T, decltype(std::declval<T>().Serialize())> : std::true_type { };

		//template <typename T, typename = ArchiveWriter&> struct GlobalHasSerialize : std::false_type { };
		//template <typename T> struct GlobalHasSerialize<T, decltype(Serialize())> : std::true_type { };


		/*template <class T>
		struct GlobalHasSerialize
		{
			static const bool value = std::is_invocable_v<decltype(&::Serialize), ArchiveWriter&, T*>;
		};*/

		template<typename T>
		inline void ArchiveWriter::Serialize(Key key, const T& data)
		{
			if constexpr (ClassHasSerialize<T*>::value)
				data->Serialize(*this);
			//else if constexpr (GlobalHasSerialize<T*>::value)
			//	::Serialize(*this, data);
			else
				Serialize(key, reinterpret_cast<const char*>(&data), sizeof(T));
		}

		template<>
		inline void ArchiveWriter::Serialize(Key key, const std::string& data)
		{
			ValuePos size = data.size();
			if (EnoughCapacityOrReserve(sizeof(ValuePos) + size))
			{
				AddVTableEntry(key);
				WriteDirect(&size, sizeof(ValuePos));
				WriteDirect(data.data(), data.size());
			}
			// ELSE:
		}

		template<>
		inline void ArchiveWriter::Serialize(Key key, const std::string_view& data)
		{
			ValuePos size = data.size();
			if (EnoughCapacityOrReserve(sizeof(ValuePos) + size))
			{
				AddVTableEntry(key);
				WriteDirect(&size, sizeof(ValuePos));
				WriteDirect(data.data(), data.size());
			}
			// ELSE:
		}


		template<>
		inline void ArchiveWriter::Serialize(Key key, const float& data)
		{
			auto value = details::CorrectEndian(data);
			AddKeyAndWrite(key, &value, sizeof(float));
		}

		template<>
		inline void ArchiveWriter::Serialize(Key key, const double& data)
		{
			auto value = details::CorrectEndian(data);
			AddKeyAndWrite(key, &value, sizeof(double));
		}

		template<>
		inline void ArchiveWriter::Serialize(Key key, const int8_t& data)
		{
			AddKeyAndWrite(key, &data, sizeof(int8_t));
		}

		template<>
		inline void ArchiveWriter::Serialize(Key key, const int16_t& data)
		{
			int16_t value = details::CorrectEndian(data);
			AddKeyAndWrite(key, &value, sizeof(int16_t));
		}

		template<>
		inline void ArchiveWriter::Serialize(Key key, const int32_t& data)
		{
			int32_t value = details::CorrectEndian(data);
			AddKeyAndWrite(key, &value, sizeof(int32_t));
		}

		template<>
		inline void ArchiveWriter::Serialize(Key key, const int64_t& data)
		{
			int64_t value = details::CorrectEndian(data);
			AddKeyAndWrite(key, &value, sizeof(int64_t));
		}

		template<>
		inline void ArchiveWriter::Serialize(Key key, const uint8_t& data)
		{
			AddKeyAndWrite(key, &data, sizeof(uint8_t));
		}

		template<>
		inline void ArchiveWriter::Serialize(Key key, const uint16_t& data)
		{
			auto value = details::CorrectEndian(data);
			AddKeyAndWrite(key, &value, sizeof(uint16_t));
		}

		template<>
		inline void ArchiveWriter::Serialize(Key key, const uint32_t& data)
		{
			auto value = details::CorrectEndian(data);
			AddKeyAndWrite(key, &value, sizeof(uint32_t));
		}

		template<>
		inline void ArchiveWriter::Serialize(Key key, const uint64_t& data)
		{
			auto value = details::CorrectEndian(data);
			AddKeyAndWrite(key, &value, sizeof(uint64_t));
		}

		template<>
		inline void ArchiveWriter::Serialize(Key key, const std::istream& data)
		{
			std::streambuf* sbuf = data.rdbuf();

			std::streamsize offset = sbuf->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
			ValuePos size = sbuf->pubseekoff(0, std::ios_base::end, std::ios_base::in) - offset;
			sbuf->pubseekpos(offset, std::ios_base::in);

			if (EnoughCapacityOrReserve(size + sizeof(ValuePos)))
			{
				AddVTableEntry(key);

				char* dataPtr = buffer + writePosition;
				memcpy(dataPtr, &size, sizeof(ValuePos));
				dataPtr += sizeof(ValuePos);

				sbuf->sgetn(dataPtr, size);

				writePosition += size + sizeof(ValuePos);
			}
			// ELSE:
		}

		template<>
		inline void ArchiveWriter::Serialize(Key key, const ArchiveWriter& data)
		{

			ValuePos contentSize = data.bufferSize;
			Key tableSize = data.table.rows.size();

			ValuePos size = sizeof(ValuePos) + contentSize + sizeof(Key) + tableSize;
			if (EnoughCapacityOrReserve(size))
			{
				AddVTableEntry(key);
				char* dataPtr = buffer + writePosition;
				
				memcpy(dataPtr, &contentSize, sizeof(ValuePos));
				dataPtr += sizeof(ValuePos);

				memcpy(dataPtr, data.buffer, contentSize);
				dataPtr += contentSize;

				memcpy(dataPtr, &tableSize, sizeof(Key));
				dataPtr += sizeof(Key);

				memcpy(dataPtr, data.table.rows.data(), tableSize * sizeof(ValuePos));

				writePosition += size;
			}
			// ELSE:
		}

		template<typename It>
		inline void ArchiveWriter::Serialize(Key key, It begin, It end)
		{
			typedef typename std::iterator_traits<It>::value_type T;
			ValuePos size = std::distance(begin, end) * sizeof(T);

			if (EnoughCapacityOrReserve(size))
			{
				AddVTableEntry(key);

				for (; begin != end; ++begin)
				{
					if (std::is_invocable_v<decltype(&T::Serialize), T*, ArchiveWriter&>)
						begin->Serialize(*this);
					else if (std::is_invocable_v<decltype(&::Serialize), ArchiveWriter&, T&>)
						::Serialize(*this, *begin);
					else
						Serialize(*begin);
				}
			}
			// ELSE:
		}


		inline void ArchiveReader::DeSerialize(Key key, void* data, std::size_t size)
		{
			ValuePos position = table->GetRow(key);
			memcpy(data, buffer + position, size);
		}

		template<typename T>
		inline void ArchiveReader::DeSerialize(Key key, T& data)
		{
			ValuePos position = table->GetRow(key);
			data = reinterpret_cast<T&>(buffer[position]);
		}

		template<>
		inline void ArchiveReader::DeSerialize(Key key, std::string& data)
		{
			ValuePos position = table->GetRow(key);
			char* bufferPart = buffer + position;

			std::size_t size = reinterpret_cast<ValuePos&>(*bufferPart);
			bufferPart += sizeof(ValuePos);

			data = std::string(bufferPart, size);
		}

		template<>
		inline void ArchiveReader::DeSerialize(Key key, std::string_view& data)
		{
			ValuePos position = table->GetRow(key);
			char* bufferPart = buffer + position;

			std::size_t size = reinterpret_cast<ValuePos&>(*bufferPart);
			bufferPart += sizeof(ValuePos);

			data = std::string_view(bufferPart, size);
		}
	}
}