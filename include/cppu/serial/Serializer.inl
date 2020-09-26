#pragma once

#include "./Serializer.h"

#include <type_traits>
#include "../type_traits.h"

namespace cppu
{
	namespace serial
	{
		namespace impl
		{
			template <typename T>
			constexpr auto template_hash() noexcept
			{
#ifdef __clang__
				const char* name = __PRETTY_FUNCTION__;
				return cppu::impl::hash_function(name + strlen("auto cppu::serial::impl::template_name() [T = "), strchr(name, '<'));
#elif defined(__GNUC__)
				const char* name = __PRETTY_FUNCTION__;
				return cppu::impl::hash_function(name + strlen("constexpr auto cppu::serial::impl::template_name() [with T = "), strchr(name, '<'));
#elif defined(_MSC_VER)
				const char* name = __FUNCSIG__;
				name += strlen("auto __cdecl cppu::serial::impl::template_name<class ");
				return cppu::impl::hash_function(name, strchr(name, '<'));
#else
				static_assert(false, "no implementation found");
#endif
			}

			template <typename T>
			constexpr auto class_hash() noexcept
			{
#ifdef __clang__
				const char* name = __PRETTY_FUNCTION__;
				return cppu::impl::hash_function(name + strlen("auto cppu::serial::impl::class_hash() [T = "), name + strlen(name) - strlen("]"));
#elif defined(__GNUC__)
				const char* name = __PRETTY_FUNCTION__;
				return cppu::impl::hash_function(name + strlen("constexpr auto cppu::serial::impl::class_hash() [with T = "), name + strlen(name) - strlen("]"));
#elif defined(_MSC_VER)
				const char* name = __FUNCSIG__;
				return cppu::impl::hash_function(name + strlen("auto __cdecl cppu::serial::impl::class_hash<class "), name + strlen(name) - strlen(">(void) noexcept"));
#else
				static_assert(false, "no implementation found");
#endif
			}

			constexpr cppu::hash_t hash_combine(cppu::hash_t hash1, cppu::hash_t hash2)
			{
				return hash2 ^ hash1 + 0x9e3779b9 + (hash2 << 6) + (hash2 >> 2);
			}

			template <typename T>
			constexpr void WriteBase(ArchiveWriter& writer, const base<T>& type)
			{
				type.ptr->T::Serialize(writer);
			}

			template <typename T>
			constexpr void ReadBase(ArchiveReader& reader, base<T>& type)
			{
				type.ptr->T::DeSerialize(reader);
			}
		}

		inline void Serialize() {}
		inline void DeSerialize() {}



		template <class T>
		inline void MemcpySerialize(cppu::serial::ArchiveWriter& writer, const T& data)
		{
			static_assert(!std::is_polymorphic_v<T>, "Serialize function is missing, as it's a polymorphic type using memcpy will bring undefined behavior. <check below/output>");

			writer.Write(&data, sizeof(T));
		}

		template <class T>
		inline void MemcpyDeSerialize(cppu::serial::ArchiveReader& reader, T& data)
		{
			static_assert(!std::is_polymorphic_v<T>, "DeSerialize function is missing, as it's a polymorphic type using memcpy will bring undefined behavior. <check below/output>");

			reader.Read(&data, sizeof(T));
		}

		inline void Construct() {}

		template <class T, std::enable_if_t<std::is_constructible_v<T> || std::is_constructible_v<T, cppu::serial::ArchiveReader&>, int> = 0>
		inline void Construct(cppu::serial::ArchiveReader& reader, cgc::strong_ptr<T>& ptr)
		{
			if constexpr (std::is_constructible_v<T, cppu::serial::ArchiveReader&>)
				ptr = cgc::construct_new<T>(reader);
			else
			{
				ptr = cgc::construct_new<T>();
				reader.Read(*ptr);
			}
		}

		template <class T, std::enable_if_t<std::is_constructible_v<T> || std::is_constructible_v<T, cppu::serial::ArchiveReader&>, int> = 0>
		inline void Construct(cppu::serial::ArchiveReader& reader, std::shared_ptr<T>& ptr)
		{
			if constexpr (std::is_constructible_v<T, cppu::serial::ArchiveReader&>)
				ptr = std::make_shared<T>(reader);
			else
			{
				ptr = std::make_shared<T>();
				reader.Read(*ptr);
			}
		}


		template <typename Type>
		class serialize_checker
		{
			// This type won't compile if the second template parameter isn't of type T,
			// so I can put a function pointer type in the first parameter and the function
			// itself in the second thus checking that the function has a specific signature.
			template <typename T, T> struct TypeCheck;

			typedef char Yes;
			typedef long No;


			template <typename T, typename P> struct ClassConstruct { typedef void (T::* func_ptr)(ArchiveReader&, P&); };
			template <typename T, typename P> static Yes HasConstruct(TypeCheck<typename ClassConstruct<T, P>::func_ptr, &T::Construct>*);
			template <typename T, typename P> static No  HasConstruct(...);

			template <typename T> struct ClassSerialize { typedef void (T::* func_ptr)(ArchiveWriter&) const; };
			template <typename T> static Yes HasSerialize(TypeCheck<typename ClassSerialize<T>::func_ptr, &T::Serialize>*);
			template <typename T> static No  HasSerialize(...);

			template <typename T> struct ClassDeSerialize { typedef void (T::* func_ptr)(ArchiveReader&); };
			template <typename T> static Yes HasDeSerialize(TypeCheck<typename ClassDeSerialize<T>::func_ptr, &T::DeSerialize>*);
			template <typename T> static No  HasDeSerialize(...);

			template <typename T> static Yes NamespaceHasSerialize(TypeCheck<typedef void(ArchiveWriter&, const T&), &Serialize>*);
			template <typename T> static No  NamespaceHasSerialize(...);

			template <typename T> static Yes NamespaceHasDeSerialize(TypeCheck<typedef void(ArchiveReader&, T&), &DeSerialize>*);
			template <typename T> static No  NamespaceHasDeSerialize(...);


		public:
			template <typename P>
			static bool const class_construct_value = sizeof(HasConstruct<Type, P>(0)) == sizeof(Yes);
			static bool const serialize_value = sizeof(HasSerialize<Type>(0)) == sizeof(Yes);
			static bool const deserialize_value = sizeof(HasDeSerialize<Type>(0)) == sizeof(Yes);

			static bool const namespace_serialize_value = sizeof(NamespaceHasSerialize<Type>(0)) == sizeof(Yes);
			static bool const namespace_deserialize_value = sizeof(NamespaceHasDeSerialize<Type>(0)) == sizeof(Yes);
		};

		template<typename T, typename P> inline constexpr bool class_has_construct_v = serialize_checker<T>::template class_construct_value<P>;
		template<typename T> inline constexpr bool class_has_serialize_v = serialize_checker<T>::serialize_value;
		template<typename T> inline constexpr bool class_has_deserialize_v = serialize_checker<T>::deserialize_value;

		template<typename T> inline constexpr bool namespace_has_serialize_v = serialize_checker<T>::namespace_serialize_value;
		template<typename T> inline constexpr bool namespace_has_deserialize_v = serialize_checker<T>::namespace_deserialize_value;

		template <typename T, typename = void> struct namespace_has_construct : std::false_type {};
		template <typename T> struct namespace_has_construct<T, std::void_t<decltype(Construct(std::declval<ArchiveReader&>(), std::declval<T&>())) >> : std::true_type {};
		template<typename T> inline constexpr bool namespace_has_construct_v = namespace_has_construct<T>::value;

		namespace impl
		{
			template <typename T>
			inline void ImplSerialize(ArchiveWriter& reader, const T& data)
			{
				if constexpr (namespace_has_serialize_v<T>)
					Serialize(reader, data);
				else
					serial::MemcpySerialize(reader, data);
			}

			template <typename T>
			inline void ImplDeSerialize(ArchiveReader& reader, T& data)
			{
				if constexpr (class_has_deserialize_v<T>)
					data.DeSerialize(reader);
				else if constexpr (namespace_has_deserialize_v<T>)
					DeSerialize(reader, data);
				else
					serial::MemcpyDeSerialize(reader, data);
			}
		}

		template <class T>
		inline void Construct(cppu::serial::ArchiveReader& reader, T*& ptr)
		{
			ptr = new std::remove_pointer_t<T>();
			impl::ImplDeSerialize(reader, *ptr);
		}

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
			ValueSize count = std::distance(begin, end);

			EnoughCapacityOrReserve(sizeof(ValuePos));

			reinterpret_cast<ValueSize&>(buffer[writePosition]) = count; // store count of array/list
			writePosition += sizeof(ValueSize);

			if constexpr (std::is_fundamental_v<T>)
			{
				if (EnoughCapacityOrReserve(count * sizeof(T)))
				{
					reinterpret_cast<ValueSize&>(buffer[writePosition]) = sizeof(T);
					writePosition += sizeof(ValueSize);

					for (; begin != end; ++begin)
						WriteDirect(&(*begin), sizeof(T));

				}
			}
			else
			{
				for (; begin != end; ++begin)
				{
					ValueSize sizePosition = writePosition;
					writePosition += sizeof(ValueSize);

					Write(*begin);

					reinterpret_cast<ValueSize&>(buffer[sizePosition]) = writePosition - sizePosition - sizeof(ValuePos);
				}

				return true;
			}

			return false;
		}

		template<typename T>
		inline bool ArchiveWriter::Write(const std::vector<T>& vector)
		{
			return Write(vector.begin(), vector.end());
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
			// pointer types
			else if constexpr (std::is_pointer_v<T>)
			{
				if (!data)
					return false;

				void* key = &*data;
				Reference reference;

				auto found = referencesTaken.find(key);
				if (found == referencesTaken.end())
				{
					const std::remove_pointer_t<T>& object = *data;
					reference = static_cast<Reference>(references.size());

					referencesTaken.emplace(key, reference);
					references.emplace_back(reference, [this, object]() { this->Write(object); });
				}
				else
					reference = found->second;

				EnoughCapacityOrReserve(sizeof(Reference));
				WriteDirect(&reference, sizeof(Reference));
			}
			// smart pointer types
			else if constexpr (::cppu::is_smart_ptr_v<T>)
			{
				if (!data)
					return false;

				void* key = &*data;
				Reference reference;
				if (referencesTaken.count(key) == 0)
				{
					typedef typename T::element_type ElementType;
					auto& object = *data;
					reference = static_cast<Reference>(references.size());

					referencesTaken.emplace(key, reference);
					references.emplace_back(reference, [this, &object = object]() { this->Write(object); });
				}
				else
					reference = referencesTaken.at(key);

				EnoughCapacityOrReserve(sizeof(Reference));
				WriteDirect(&reference, sizeof(Reference));
			}
			// all other value types
			else
			{
				if constexpr (std::is_class_v<T>)
				{
					if constexpr (std::is_polymorphic_v<T>)
					{
						auto& classTable = Serializer::ClassIDs();
						auto found = classTable.find(typeid(data));
						Write(static_cast<Reference>(found != classTable.end() ? found->second : 0));
					}
				}

				if constexpr (class_has_serialize_v<T>)
					data.Serialize(*this);
				else
					impl::ImplSerialize(*this, data);
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

		template<typename... P>
		inline void ArchiveWriter::Polymorphic(const P... bases)
		{
			//bool success = Write(impl::class_hash<T>());

			// fold expression
			(impl::WriteBase(*this, bases), ...);
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

		template<typename T>
		inline bool SubArchiveWriter::Write(const std::vector<T>& vector)
		{
			return writer.Write(vector);
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

		template<typename... P>
		inline void SubArchiveWriter::Polymorphic(const P... bases)
		{
			writer.Polymorphic(bases...);
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

				using ElementType = typename std::pointer_traits<T>::element_type;

				Reference referenceIndex = reinterpret_cast<Reference&>(buffer[position]);

				VReferenceTableRead::Row reference = reinterpret_cast<VReferenceTableRead::Row&>((&referenceTable->rows)[referenceIndex]);
				auto found = references->find(reference.id);
				if (found != references->end())
				{
					if constexpr (std::is_pointer_v<T>)
						data = found->second.operator() < ElementType* > ();
					else
						data = found->second.operator() < T > ();
				}
				else
				{

					ValuePos oldReadPosition = readPosition;

					readPosition = reference.position;

					//hash_t hash = impl::class_hash<ElementType>();
					if (std::is_class_v<ElementType> && std::is_polymorphic_v<ElementType>)
					{
						hash_t hash = reinterpret_cast<Reference&>(buffer[readPosition]);
						readPosition += sizeof(Reference);
						if (hash == 0)
							goto base_class_deserialize;


						if constexpr (::cppu::is_smart_ptr_v<T>)
							hash = impl::hash_combine(hash, impl::template_hash<T>());

						//Serializer::Register<ElementType>();

						auto& constructors = Serializer::ClassConstructors();

						auto constructorFound = constructors.find(hash);
						if (constructorFound != constructors.end())
						{
							// the inside function will turn it back
							constructorFound->second(*this, reinterpret_cast<void*>(&data));
						}
					}
					else
					{
					base_class_deserialize:

						if constexpr (class_has_construct_v<ElementType, ElementType>)
							ElementType::Construct(*this, data);
						else
							Construct(*this, data);
					}

					readPosition = oldReadPosition;

					if constexpr (!::cppu::is_smart_ptr_v<T> && std::is_pointer_v<T>)
						references->try_emplace(reference.id, false, data, sizeof(T));
					else
						references->try_emplace(reference.id, true, reinterpret_cast<void*>(&data), sizeof(T));
				}

				// make sure we can continue our previous deserialization
			}
			// container types
			else if constexpr (::cppu::is_container_v<T> && !cppu::is_string<T>::value)
			{
				ValueSize count;
				ReadPosition(count, position);
				readPosition = position + sizeof(ValueSize);

				if constexpr (cppu::is_map_v<T>)
				{
					for (uint32_t i = 0; i < count; ++i)
					{
						ValuePos size;
						ReadPosition(size, readPosition);
						size += readPosition; // just reuse the same memory

						typename T::key_type key;
						Read(key);

						typename T::mapped_type value;
						Read(value);

						// TODO: allow in place construction
						data.try_emplace(key, value);

						readPosition = size;
					}
				}
				else if constexpr (cppu::is_vector_v<T>)
				{
					if (data.size() < count)
						data.resize(count);

					for (uint32_t i = 0; i < count; ++i)
					{
						ValueSize size = reinterpret_cast<ValueSize&>(buffer[readPosition]);
						readPosition += sizeof(ValueSize);

						ValuePos nextPos = readPosition + size; // just reuse the same memory

						if (size)
							ReadPosition(data[i], readPosition);

						readPosition = nextPos;
					}
				}
			}
			// all other value types
			else
			{
				readPosition = position;
				if constexpr (std::is_class_v<T>)
				{
					if constexpr (std::is_polymorphic_v<T>)
					{
						Reference refIndex;
						Read(refIndex);
					}
				}

				impl::ImplDeSerialize(*this, data);
			}
		}

		template<typename K, typename T>
		inline void ArchiveReader::ReadPosition(std::pair<K, T>& data, int position)
		{
			readPosition = position;
			Read(data.first);
			Read(data.second);
		}

		template<typename... TT>
		inline void ArchiveReader::ReadPosition(std::tuple<TT...>& data, int position)
		{
			readPosition = position;
			std::apply(Read, data);
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
			ValuePos position = table->GetRow(key);
			ReadPosition<T>(data, position);
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

		template<typename... P>
		inline void ArchiveReader::Polymorphic(P... bases)
		{
			// fold expression
			(impl::ReadBase(*this, bases), ...);
		}

		inline ArchiveReader ArchiveReader::GetSubArchive()
		{
			ValuePos startPosition = readPosition;

			ValuePos size = reinterpret_cast<ValuePos&>(buffer[readPosition]);
			readPosition += sizeof(ValuePos);
			ArchiveVersion version = reinterpret_cast<ArchiveVersion&>(buffer[readPosition + sizeof(ValuePos)]);
			//readPosition += sizeof(ArchiveVersion);

			//readPosition += size; // advance past all the contents

			VTableRead* vTable = reinterpret_cast<VTableRead*>(buffer + startPosition + size);

			readPosition = startPosition + size + sizeof(ValueSize) + vTable->size * sizeof(ValuePos); // advance past the vtable

			return ArchiveReader(buffer, size, startPosition, vTable, referenceTable, references);

		}
#pragma endregion

#pragma region Serializer
		template<typename T>
		inline bool Serializer::Register()
		{
			if constexpr (std::is_polymorphic_v<T>)
			{
				if constexpr (class_has_construct_v<T, T>)
					static auto raw_ptr_constructor = ClassConstructors().emplace(impl::class_hash<T>(),
						[](ArchiveReader& reader, void* ptr) { T::Construct(reader, **reinterpret_cast<T**>(ptr)); });
				else if constexpr (namespace_has_construct_v<T>)
					static auto raw_ptr_constructor = ClassConstructors().emplace(impl::class_hash<T>(),
						[](ArchiveReader& reader, void* ptr) { Construct(reader, **reinterpret_cast<T**>(ptr)); });
				else if constexpr (std::is_constructible_v<T, ArchiveReader&>)
					static auto raw_ptr_constructor = ClassConstructors().emplace(impl::class_hash<T>(),
						[](ArchiveReader& reader, void* ptr) { T** rawPtr = reinterpret_cast<T**>(ptr); *rawPtr = new T(reader); });
				else if constexpr (std::is_constructible_v<T>)
					static auto raw_ptr_constructor = ClassConstructors().emplace(impl::class_hash<T>(),
						[](ArchiveReader& reader, void* ptr) { T** rawPtr = reinterpret_cast<T**>(ptr); *rawPtr = new T(); reader.Read(**rawPtr); });
				if constexpr (class_has_construct_v<T, cppu::cgc::strong_ptr<T>>)
					static auto cgc_smart_ptr_constructor = ClassConstructors().emplace(impl::hash_combine(impl::class_hash<T>(), impl::template_hash<cppu::cgc::strong_ptr<T>>()),
						[](ArchiveReader& reader, void* ptr) { T::Construct(reader, *reinterpret_cast<cppu::cgc::strong_ptr<T>*>(ptr)); });
				else if constexpr (namespace_has_construct_v<cppu::cgc::strong_ptr<T>>)
					static auto cgc_smart_ptr_constructor = ClassConstructors().emplace(impl::hash_combine(impl::class_hash<T>(), impl::template_hash<cppu::cgc::strong_ptr<T>>()),
						[](ArchiveReader& reader, void* ptr) { Construct(reader, *reinterpret_cast<cppu::cgc::strong_ptr<T>*>(ptr)); });
				else if constexpr (std::is_constructible_v<T, ArchiveReader&>)
				{
					static auto cgc_smart_ptr_constructor = ClassConstructors().emplace(impl::hash_combine(impl::class_hash<T>(), impl::template_hash<cppu::cgc::strong_ptr<T>>()),
						[](ArchiveReader& reader, void* ptr) {
						cgc::strong_ptr<T>& smartPtr = *reinterpret_cast<cgc::strong_ptr<T>*>(ptr);
						smartPtr = cgc::construct_new<T>(reader);
					});
				}
				else if constexpr (std::is_constructible_v<T>)
				{
					static auto cgc_smart_ptr_constructor = ClassConstructors().emplace(impl::hash_combine(impl::class_hash<T>(), impl::template_hash<cppu::cgc::strong_ptr<T>>()),
						[](ArchiveReader& reader, void* ptr) {
						cgc::strong_ptr<T>& smartPtr = *reinterpret_cast<cgc::strong_ptr<T>*>(ptr);
						smartPtr = cgc::construct_new<T>();
						reader.Read(*smartPtr);
					});
				}

				if constexpr (class_has_construct_v<T, std::shared_ptr<T>>)
					static auto std_smart_ptr_constructor = ClassConstructors().emplace(impl::hash_combine(impl::class_hash<T>(), impl::template_hash<std::shared_ptr<T>>()),
						[](ArchiveReader& reader, void* ptr) { T::Construct(reader, *reinterpret_cast<std::shared_ptr<T>*>(ptr)); });
				else if constexpr (namespace_has_construct_v<std::shared_ptr<T>>)
					static auto std_smart_ptr_constructor = ClassConstructors().emplace(impl::hash_combine(impl::class_hash<T>(), impl::template_hash<std::shared_ptr<T>>()),
						[](ArchiveReader& reader, void* ptr) { Construct(reader, *reinterpret_cast<std::shared_ptr<T>*>(ptr)); });
				else if constexpr (std::is_constructible_v<T, ArchiveReader&>)
				{
					static auto std_smart_ptr_constructor = ClassConstructors().emplace(impl::hash_combine(impl::class_hash<T>(), impl::template_hash<std::shared_ptr<T>>()),
						[](ArchiveReader& reader, void* ptr) {
						std::shared_ptr<T>& smartPtr = *reinterpret_cast<std::shared_ptr<T>*>(ptr);
						smartPtr = std::make_shared<T>(reader);
					});
				}
				else if constexpr (std::is_constructible_v<T>)
				{
					static auto std_smart_ptr_constructor = ClassConstructors().emplace(impl::hash_combine(impl::class_hash<T>(), impl::template_hash<std::shared_ptr<T>>()),
						[](ArchiveReader& reader, void* ptr) {
						std::shared_ptr<T>& smartPtr = *reinterpret_cast<std::shared_ptr<T>*>(ptr);
						smartPtr = std::make_shared<T>();
						reader.Read(*smartPtr);
					});
				}

				auto key = std::type_index(typeid(T));
				auto val = impl::class_hash<T>();

				Serializer::ClassIDs().emplace(key, val);
			}
			else
			{
#define TYPE_NAME_ERROR(N) "Type #N is not a polymorphic type, consider making this type a polymorphic type or remove the registration."
				static_assert(false, TYPE_NAME_ERROR(T));
			}

			return true;
		}
#pragma endregion
	}
}