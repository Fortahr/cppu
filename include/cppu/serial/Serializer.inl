#pragma once

#include "./Serializer.h"

#include <type_traits>
#include "../type_traits.h"

namespace cppu
{
	namespace serial
	{
		template <typename Type>
		class serialize_checker
		{
			// This type won't compile if the second template parameter isn't of type T,
			// so I can put a function pointer type in the first parameter and the function
			// itself in the second thus checking that the function has a specific signature.
			template <typename T, T> struct TypeCheck;

			typedef char Yes;
			typedef long No;

			// A helper struct to hold the declaration of the function pointer.
			// Change it if the function signature changes.
			template <typename T> struct Construct { typedef void (T::* func_ptr)(ArchiveWriter&, T&) const; };
			template <typename T> static Yes HasConstruct(TypeCheck<typename Construct<T>::func_ptr, &T::Construct>*);
			template <typename T> static No  HasConstruct(...);

			template <typename T> struct Serialize { typedef void (T::*func_ptr)(ArchiveWriter&) const; };
			template <typename T> static Yes HasSerialize(TypeCheck<typename Serialize<T>::func_ptr, &T::Serialize>*);
			template <typename T> static No  HasSerialize(...);

			template <typename T> struct DeSerialize { typedef void (T::* func_ptr)(ArchiveReader&); };
			template <typename T> static Yes HasDeSerialize(TypeCheck<typename DeSerialize<T>::func_ptr, &T::DeSerialize>*);
			template <typename T> static No  HasDeSerialize(...);

		public:
			static bool const construct_value = sizeof(HasConstruct<Type>(0)) == sizeof(Yes);
			static bool const serialize_value = sizeof(HasSerialize<Type>(0)) == sizeof(Yes);
			static bool const deserialize_value = sizeof(HasDeSerialize<Type>(0)) == sizeof(Yes);
		};

		template<typename T> inline constexpr bool class_has_construct_v = serialize_checker<T>::construct_value;
		template<typename T> inline constexpr bool class_has_serialize_v = serialize_checker<T>::serialize_value;
		template<typename T> inline constexpr bool class_has_deserialize_v = serialize_checker<T>::deserialize_value;

		/*template <class T>
		void Serialize(ArchiveWriter& writer, T& data)
		{
		}

		template <class T>
		void DeSerialize(ArchiveReader& reader, T& data)
		{

		}*/

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


#pragma region Writer

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

				if (!references.empty())
				{

					ValuePos referencesSizePosition = writePosition;
					writePosition += sizeof(ValueSize);

					cppu::stor::vector<char> vtable;
					uint32_t referenceSize = references.size();
					vtable.resize_no_construct(referenceSize * (sizeof(ValuePos) + sizeof(Reference)));
					char* vtableWritePtr = vtable.data();

					do
					{
						//EnoughCapacityOrReserve(sizeof(ValueSize));
						ValuePos sizePosition = writePosition;
						writePosition += sizeof(ValuePos);

						auto& ref = references.front();
						ref.serialize();

						if (referencesTaken.size() > referenceSize)
						{
							referenceSize = referencesTaken.size();
							vtable.resize_no_construct(referenceSize * (sizeof(ValuePos) + sizeof(Reference)));
						}

						reinterpret_cast<ValueSize&>(buffer[sizePosition]) = writePosition - sizePosition - sizeof(ValueSize);

						reinterpret_cast<ValuePos&>(*vtableWritePtr) = sizePosition + sizeof(ValueSize);
						vtableWritePtr += sizeof(ValuePos);
						reinterpret_cast<Reference&>(*vtableWritePtr) = ref.key;
						vtableWritePtr += sizeof(Reference);

						references.pop_front();
					} while (!references.empty());


					reinterpret_cast<ValuePos&>(buffer[referencesSizePosition]) = writePosition;

					EnoughCapacityOrReserve(vtable.size());

					reinterpret_cast<ValueSize&>(buffer[writePosition]) = static_cast<ValueSize>(referencesTaken.size());
					writePosition += sizeof(ValueSize);

					memcpy(buffer + writePosition, vtable.data(), vtable.size());
					writePosition += vtable.size();
				}
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
			return writePosition + size <= bufferSize || Reserve(writePosition + size);
		}

		inline bool ArchiveWriter::Serialize(Key key, const void* data, std::size_t size)
		{
			AddVTableEntry(key);

			if (EnoughCapacityOrReserve(size))
			{
				WriteDirect(&data, size);
				return true;
			}

			return false;
		}

		template<typename T>
		inline bool ArchiveWriter::Serialize(Key key, const T& data)
		{
			AddVTableEntry(key);
			return Write(data);
		}

		template<typename It>
		inline bool ArchiveWriter::Serialize(Key key, It begin, It end)
		{
			AddVTableEntry(key);
			return Write(begin, end);
		}

		template<typename It>
		inline bool ArchiveWriter::Write(It begin, It end)
		{
			typedef typename std::iterator_traits<It>::value_type T;
			ValuePos count = std::distance(begin, end);

			EnoughCapacityOrReserve(sizeof(ValuePos));

			reinterpret_cast<ValuePos&>(buffer[writePosition]) = count; // store count of array/list
			ValuePos objectSizePosition = writePosition + sizeof(ValuePos);
			writePosition += sizeof(ValuePos);

			if constexpr (std::is_fundamental_v<T>)
			{
				if (EnoughCapacityOrReserve(count * sizeof(T)))
				{
					for (; begin != end; ++begin)
						WriteDirect(&(*begin), sizeof(T));

					reinterpret_cast<ValuePos&>(buffer[objectSizePosition]) = sizeof(T);
				}
			}
			else
			{
				for (; begin != end; ++begin)
				{
					ValuePos sizePosition = writePosition;
					writePosition += sizeof(ValuePos);

					Write(*begin);

					reinterpret_cast<ValuePos&>(buffer[sizePosition]) = writePosition - sizePosition - sizeof(ValuePos);
				}

				return true;
			}
			
			return false;
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

		inline bool ArchiveWriter::Write(const void* data, std::size_t size)
		{
			if (EnoughCapacityOrReserve(size))
			{
				WriteDirect(data, size);
				return true;
			}

			return false;
		}

		template<typename T>
		inline bool ArchiveWriter::Write(const T& data)
		{			
			// basic fundamental types
			if constexpr (std::is_fundamental_v<T>)
			{
				if (EnoughCapacityOrReserve(sizeof(T)))
				{
					if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
					{
						auto value = details::CorrectEndian(data);
						WriteDirect(&value, sizeof(T));
					}
					else
						WriteDirect(&data, sizeof(T));

					return true;
				}

				return false;
			}
			// reference / pointer types
			else if constexpr (std::is_pointer_v<T>)
			{
				std::size_t key = reinterpret_cast<const std::size_t&>(data);
				Reference reference;

				auto found = referencesTaken.find(key);
				if (found == referencesTaken.end())
				{
					const std::remove_pointer_t<T>& object = *data;
					reference = static_cast<Reference>(references.size());

					referencesTaken.emplace(key, reference);
					references.emplace_back(key, [this, object]() { this->Write(object); });
				}
				else
					reference = found->second;

				EnoughCapacityOrReserve(sizeof(Reference));
				WriteDirect(&reference, sizeof(Reference));
			}
			// smart pointer types
			else if constexpr (::cppu::is_smart_ptr_v<T>)
			{
				std::size_t key = reinterpret_cast<std::size_t&>(*data);
				Reference reference;
				if (referencesTaken.count(key) == 0)
				{
					typedef typename T::element_type ElementType;
					auto& object = *data;
					reference = static_cast<Reference>(references.size());

					referencesTaken.emplace(key, reference);
					references.emplace_back(key, [this, object]() { this->Write(object); });
				}
				else
					reference = referencesTaken.at(key);

				EnoughCapacityOrReserve(sizeof(Reference));
				WriteDirect(&reference, sizeof(Reference));
			}
			// all other value types
			else
			{
				
				if constexpr (class_has_serialize_v<T>)
					data.Serialize(*this);
				else
					serial::Serialize(*this, data);
			}

			return false;
		}

		template<typename K, typename T>
		inline bool ArchiveWriter::Write(const std::pair<K, T>& data)
		{
			Write(data.first);
			return Write(data.second);
		}

		template<class... TT>
		inline bool ArchiveWriter::Write(const std::tuple<TT...>& data)
		{
			std::apply(Write, data);
		}

		template<>
		inline bool ArchiveWriter::Write(const std::string& data)
		{
			ValuePos size = data.size();
			if (EnoughCapacityOrReserve(sizeof(ValuePos) + size))
			{
				WriteDirect(&size, sizeof(ValuePos));
				WriteDirect(data.data(), data.size());

				return true;
			}

			return false;
		}

		template<>
		inline bool ArchiveWriter::Write(const std::string_view& data)
		{
			ValuePos size = data.size();
			if (EnoughCapacityOrReserve(sizeof(ValuePos) + size))
			{
				WriteDirect(&size, sizeof(ValuePos));
				WriteDirect(data.data(), data.size());

				return true;
			}

			return false;
		}

		template<>
		inline bool ArchiveWriter::Write(const std::istream& data)
		{
			std::streambuf* sbuf = data.rdbuf();

			std::streamsize offset = sbuf->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
			ValuePos size = ValuePos(sbuf->pubseekoff(0, std::ios_base::end, std::ios_base::in) - offset);
			sbuf->pubseekpos(offset, std::ios_base::in);

			if (EnoughCapacityOrReserve(size + sizeof(ValuePos)))
			{
				char* dataPtr = buffer + writePosition;
				memcpy(dataPtr, &size, sizeof(ValuePos));
				dataPtr += sizeof(ValuePos);

				sbuf->sgetn(dataPtr, size);

				writePosition += size + sizeof(ValuePos);

				return true;
			}
			// ELSE:
			return false;
		}

		template<>
		inline bool ArchiveWriter::Write(const ArchiveWriter& data)
		{

			ValuePos contentSize = data.bufferSize;
			Key tableSize = data.table.rows.size();

			ValuePos size = sizeof(ValuePos) + contentSize + sizeof(Key) + tableSize;
			if (EnoughCapacityOrReserve(size))
			{
				char* dataPtr = buffer + writePosition;
				
				memcpy(dataPtr, &contentSize, sizeof(ValuePos));
				dataPtr += sizeof(ValuePos);

				memcpy(dataPtr, data.buffer, contentSize);
				dataPtr += contentSize;

				memcpy(dataPtr, &tableSize, sizeof(Key));
				dataPtr += sizeof(Key);

				memcpy(dataPtr, data.table.rows.data(), tableSize * sizeof(ValuePos));

				writePosition += size;

				return true;
			}
			// ELSE:

			return false;
		}

		inline SubArchiveWriter ArchiveWriter::CreateSubArchive(VTableSize tableSize, ArchiveVersion version, ValuePos initialBufferSize)
		{
			EnoughCapacityOrReserve(writePosition + initialBufferSize);

			// unrecommended way of disabling destructor call, but we need it for convenience
			char tempStorage[sizeof(SubArchiveWriter)];
			new (&tempStorage) SubArchiveWriter(tableSize, version, *this, writePosition);
			SubArchiveWriter& subWriter = reinterpret_cast<SubArchiveWriter&>(tempStorage);

			memcpy(buffer + writePosition + sizeof(ValuePos), &version, sizeof(ArchiveVersion));
			writePosition += sizeof(ValuePos) + sizeof(ArchiveVersion);

			std::swap(this->table, subWriter.swappedTable);

			return std::move(subWriter);
		}

		inline void ArchiveWriter::FinishSubArchive(SubArchiveWriter& subArchive)
		{
			if (EnoughCapacityOrReserve(sizeof(ValuePos) + table.rows.size() * sizeof(ValuePos)))
			{
				ValuePos size = writePosition - subArchive.startPosition;
				memcpy(buffer + subArchive.startPosition, &size, sizeof(writePosition));

				ValuePos vtableCount = table.rows.size();
				ValuePos vtableSize = table.rows.size() * sizeof(ValuePos);

				memcpy(buffer + writePosition, &vtableCount, sizeof(ValuePos));
				writePosition += sizeof(ValuePos);
				memcpy(buffer + writePosition, table.rows.data(), vtableSize);
				writePosition += vtableSize;
			}

			// swap old table back to continue with the older one
			table = std::move(subArchive.swappedTable);
		}

		template<typename T>
		inline bool SubArchiveWriter::Serialize(Key key, const T& data)
		{
			return writer.Serialize(key, data);
		}

		inline bool SubArchiveWriter::Serialize(Key key, const void* data, std::size_t size)
		{
			return writer.Serialize(key, data, size);
		}

		template<typename It>
		inline bool SubArchiveWriter::Serialize(Key key, It begin, It end)
		{
			return writer.Serialize(key, begin, end);
		}

		inline bool SubArchiveWriter::Write(const void* data, std::size_t size)
		{
			return writer.Write(data, size);
		}

		template<typename T>
		inline bool SubArchiveWriter::Write(const T& data)
		{
			return writer.Write(data);
		}

		template<typename It>
		inline bool SubArchiveWriter::Write(It begin, It end)
		{
			return writer.Write(begin, end);
		}

		template<typename K, typename T>
		inline bool SubArchiveWriter::Write(const std::pair<K, T>& data)
		{
			return writer.Write(data);
		}

		template<class... TT>
		inline bool SubArchiveWriter::Write(const std::tuple<TT...>& data)
		{
			return writer.Write(data);
		}

		inline SubArchiveWriter::~SubArchiveWriter()
		{
			writer.FinishSubArchive(*this);
		}

#pragma endregion
#pragma region Reader
		template<>
		inline void ArchiveReader::ReadPosition(std::string& data, int position)
		{
			ValuePos size = reinterpret_cast<ValuePos&>(buffer[position]);
			position += sizeof(ValuePos);
			data = std::string(buffer + position, size);
			readPosition = position + size;
		}

		template<typename T>
		inline void ArchiveReader::ReadPosition(T& data, int position)
		{
			// basic fundamental types
			if constexpr (std::is_fundamental_v<T>)
			{
				if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
				{
					data = details::CorrectEndian(reinterpret_cast<T&>(buffer[position]));
				}
				else
					data = reinterpret_cast<T&>(buffer[position]);

				readPosition = position + sizeof(T);
			}
			// raw and smart pointer types
			else if constexpr (std::is_pointer_v<T> || ::cppu::is_smart_ptr_v<T>)
			{
				// TODO: enable with lookup table

				typedef std::remove_pointer_t<std::decay_t<T>> ElementType;

				ValuePos referenceIndex = reinterpret_cast<ValuePos&>(buffer[position]);

				VReferenceTableRead::Row reference = reinterpret_cast<VReferenceTableRead::Row&>((&referenceTable->rows)[referenceIndex]);
				auto found = references->find(reference.id);
				if (found != references->end())
				{
					if constexpr (std::is_pointer_v<T>)
						data = static_cast<ElementType*>(found->second.ptr);
					else
						data = *reinterpret_cast<T*>(found->second.ptr);
				}
				else
				{

					ValuePos oldReadPosition = readPosition;
					
					readPosition = reference.position;
					if constexpr (class_has_construct_v<ElementType>)
						ElementType::Construct(*this, data);
					else
						serial::Construct(*this, data);

					readPosition = oldReadPosition;

					if constexpr (std::is_pointer_v<T>)
						references->try_emplace(reference.id, false, data);
					else
						references->try_emplace(reference.id, true, static_cast<void*>(&data));
				}

				// make sure we can continue our previous deserialization
			}
			// all other value types
			else
			{
				readPosition = position;

				if constexpr (class_has_deserialize_v<T>)
					data.DeSerialize(*this);
				else
					serial::DeSerialize(*this, data);
			}
		}

		inline void ArchiveReader::ReadPosition(void* data, std::size_t size, int position)
		{
			memcpy(data, buffer + position, size);
			readPosition = position + size;
		}

		template<typename T>
		inline void ArchiveReader::Read(T& data, int offset)
		{
			ReadPosition(data, readPosition + offset);
		}

		inline void ArchiveReader::Read(void* data, std::size_t size, int offset)
		{
			ReadPosition(data, size, readPosition + offset);
		}

		inline void ArchiveReader::DeSerialize(Key key, void* data, std::size_t size)
		{
			ValuePos position = table->GetRow(key);
			memcpy(data, buffer + position, size);
		}

		template<typename T>
		inline void ArchiveReader::DeSerialize(Key key, T& data)
		{
			if constexpr (cppu::is_container<T>::value && !cppu::is_string<T>::value)
				DeSerializeContainer(key, data);
			else
			{
				ValuePos position = table->GetRow(key);
				ReadPosition<T>(data, position);
			}
		}

		template<typename T>
		inline void ArchiveReader::DeSerializeContainer(Key key, T& data)
		{
			ValuePos position = table->GetRow(key);
			ValuePos size = reinterpret_cast<ValuePos&>(buffer[position]);
			data.resize(size);

			position += sizeof(ValuePos);

			for (uint32_t i = 0; i < size; ++i)
			{
				ValuePos offset = reinterpret_cast<ValuePos&>(buffer[position]);
				position += sizeof(ValuePos);
				if (offset)
					ReadPosition(data[i], position);

				position += offset;
			}
		}

		template<typename T>
		inline void ArchiveReader::DeSerialize(Key key, std::list<T>& data)
		{
			ValuePos position = table->GetRow(key);
			ValuePos size = reinterpret_cast<ValuePos&>(buffer[position]);

			position += sizeof(ValuePos);

			for (uint32_t i = 0; i < size; ++i)
			{
				ValuePos offset = reinterpret_cast<ValuePos&>(buffer[position]);
				position += sizeof(ValuePos);
				T& object = data.emplace_back();

				if (offset)
					ReadPosition(object, position);

				position += offset;
			}
		}

		inline ArchiveReader ArchiveReader::GetSubArchive()
		{
			ValuePos startPosition = readPosition;

			ValuePos size = reinterpret_cast<ValuePos&>(buffer[readPosition]);
			readPosition += sizeof(ValuePos);
			ArchiveVersion version = reinterpret_cast<ValuePos&>(buffer[readPosition + sizeof(ValuePos)]);
			readPosition += sizeof(ArchiveVersion);
			readPosition += size;

			return ArchiveReader(buffer, size, startPosition, reinterpret_cast<VTableRead*>(buffer + startPosition + size), referenceTable, references);
			
		}
#pragma endregion
	}
}