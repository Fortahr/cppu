#pragma once

namespace cppu
{
	namespace stor
	{
		template <class T, typename size_type = size_t, size_type addCapacity = 2, size_type mulCapacity = 1>
		class vector
		{
		public:
			class iterator
			{
				friend class vector;
			private:
				T* ptr;

				iterator(T* ptr)
					: ptr(ptr)
				{}

			public:
				iterator(const iterator& copy)
					: ptr(copy.ptr)
				{}

				T& operator*() { return *ptr; }
				T* operator->() { return ptr; }

				iterator& operator--() { --ptr; return *this; }
				iterator& operator-=(size_type offset) { ptr -= offset; return *this; }
				friend iterator operator-(const iterator& lhs, size_type offset) { return iterator(lhs.ptr - offset); }

				iterator& operator++() { ++ptr; return *this; }
				iterator& operator+=(size_type offset) { ptr += offset; return *this; }
				friend iterator operator+(const iterator& lhs, size_type offset) { return iterator(lhs.ptr + offset); }

				iterator& operator=(const iterator& copy) { ptr = copy.ptr; return *this; }

				bool operator==(const iterator& other) { return ptr == other.ptr; }
				bool operator!=(const iterator& other) { return ptr == other.ptr; }
			};

			class const_iterator
			{
				friend class vector;
			private:
				T* ptr;

				const_iterator(T* ptr)
					: ptr(ptr)
				{}
			public:
				const_iterator(const const_iterator& copy)
					: ptr(copy.ptr)
				{}

				const_iterator(const iterator& copy)
					: ptr(copy.ptr)
				{}

				const T& operator*() { return *ptr; }
				const T* operator->() { return ptr; }

				const_iterator& operator--() { --ptr; return *this; }
				const_iterator& operator-=(size_type offset) { ptr -= offset; return *this; }
				friend const_iterator operator-(const const_iterator& lhs, size_type offset) { return const_iterator(lhs.ptr - offset); }

				const_iterator& operator++() { ++ptr; return *this; }
				const_iterator& operator+=(size_type offset) { ptr += offset; return *this; }
				friend const_iterator operator+(const const_iterator& lhs, size_type offset) { return const_iterator(lhs.ptr + offset); }

				const_iterator& operator=(const const_iterator& copy) { ptr = copy.ptr; return *this; }
				const_iterator& operator=(const iterator& copy) { ptr = copy.ptr; return *this; }

				bool operator==(const iterator& other) { return ptr == other.ptr; }
				bool operator!=(const iterator& other) { return ptr != other.ptr; }
			};

		private:
			size_type _size;
			size_type _capacity;
			T* _data;

			static T* allocate(size_type size)
			{
				return static_cast<T*>(malloc(size * sizeof(T)));
			}

			static void copyRange(T* begin, std::size_t size, T* dest)
			{
				if constexpr (!std::is_fundamental<T>::value)
				{
					T* end = begin + size;
					while (begin != end)
					{
						new((void*)dest) T(*begin);
						++begin;
						++dest;
					}
				}
				else
					memcpy(dest, begin, size);
			}

			static void moveRange(T* begin, T* end, T* dest)
			{
				if constexpr (!std::is_fundamental<T>::value)
				{
					while (begin != end)
					{
						new((void*)dest) T(std::move(*begin));
						++begin;
						++dest;
					}
				}
				else
				{
					while (begin != end)
					{
						*dest = *begin;
						++begin;
						++dest;
					}
				}
			}
			
			static void moveRangeReverse(T* begin, T* end, T* endDest)
			{
				while (end >= begin)
				{
					new((void*)endDest) T(std::move(*end));
					--end;
					--endDest;
				}
			}

			static void deleteRange(T* begin, T* end)
			{
				if constexpr (!std::is_fundamental<T>::value)
				{
					while (begin != end)
					{
						begin->~T();
						++begin;
					}
				}
			}

			T* reallocate(size_type newCapacity)
			{
				T* newData = _data != nullptr
					? static_cast<T*>(realloc(_data, newCapacity * sizeof(T)))
					: static_cast<T*>(malloc(newCapacity * sizeof(T)));
				
				if(newData == nullptr)
					puts("Error (re)allocating memory");
				else
				{
					_data = newData;
					_capacity = newCapacity;
				}

				return newData;
			}

			static void constructRange(T* begin, T* end)
			{
				do new((void*)begin) T;
				while (++begin != end);
			}

			static void constructRange(T* begin, T* end, const T& fillWith)
			{
				do new((void*)begin) T(fillWith);
				while (++begin != end);
			}

		public:
			typedef T value_type;

			vector()
			{
				_size = 0;
				_capacity = 0;
				_data = 0;
			}

			vector(const vector<T, size_type>& copy)
				: _size(copy._size)
				, _capacity(copy._capacity)
				, _data(nullptr)
			{
				_data = allocate(_capacity);
				if (_data != nullptr)
					copyRange(copy._data, _size, _data);
				else
					_size = _capacity = 0;
			}

			vector(vector<T, size_type>&& move)
				: _size(std::move(move._size))
				, _capacity(std::move(move._capacity))
				, _data(std::move(move._data))
			{
				move._size = 0;
				move._capacity = 0;
				move._data = nullptr;
			}

			vector<T, size_type>& operator= (const vector<T, size_type>& copy)
			{
				_size = copy._size;
				_capacity = copy._capacity;
				_data = nullptr;

				_data = allocate(_capacity);
				if (_data != nullptr)
					copyRange(copy._data, _size, _data);
				else
					_size = _capacity = 0;

				return *this;
			}

			vector<T, size_type>& operator= (vector<T, size_type>&& move)
			{
				_size = std::move(move._size);
				_capacity = std::move(move._capacity);
				_data = std::move(move._data);
				
				move._size = 0;
				move._capacity = 0;
				move._data = nullptr;

				return *this;
			}
			
			~vector()
			{
				if (_data)
				{
					deleteRange(_data, _data + _size);
					free(_data);
				}
			}

			// needs some other constructors
			// size(), empty() and a bunch of other methods!

			void push_back(const T& value)
			{
				if (_size < _capacity)
				{
					new((void*)(_data + _size)) T(value);
					_size++;
					return;
				}

				size_type newCapacity = _capacity + _capacity * mulCapacity + addCapacity;
				T* newData = allocate(newCapacity);
				moveRange(_data, _data + _size, newData);
				new((void*)(newData + _size)) T(value);

				deleteRange(_data, _data + _size);
				free(_data);

				_data = newData;
				_capacity = newCapacity;
				_size++;
			}

			template<class... _Args>
			void emplace_back(_Args&& ... arguments)
			{
				if (_size < _capacity)
				{
					new((void*)(_data + _size)) T(std::forward<_Args>(arguments)...);
					_size++;
					return;
				}

				size_type newCapacity = _capacity + _capacity * mulCapacity + addCapacity;
				T* newData = allocate(newCapacity);
				moveRange(_data, _data + _size, newData);
				new((void*)(newData + _size)) T(std::forward<_Args>(arguments)...);

				deleteRange(_data, _data + _size);
				free(_data);
				_data = newData;
				_capacity = newCapacity;
				_size++;
			}

			template<class... _Args>
			void emplace(size_type index, _Args && ... arguments)
			{
					T* end = _data + _size;

				// can we just move all data (enough allocated data)?
				if (_size < _capacity)
				{
					if (index < _size)
						moveRangeReverse(_data + index, end - 1, end);

					(_data + index)->~T();
					new((void*)(_data + index)) T(std::forward<_Args>(arguments)...);

					_size++;
				}
				else
				{
					// allocate
					size_type newCapacity = _capacity + _capacity * mulCapacity + addCapacity;
					T* newData = allocate(newCapacity);

					// move data
					if (_data != nullptr)
					{
						moveRange(_data, _data + index, newData);
						moveRangeReverse(_data + index, end - 1, newData + _size);
					}

					// construct new object
					new((void*)(newData + index)) T(std::forward<_Args>(arguments)...);

					// clean up old data
					deleteRange(_data, _data + _size);
					free(_data);

					// update all variables
					_data = newData;
					_capacity = newCapacity;
					_size++;
				}
			}

			template<class... _Args>
			void emplace(const const_iterator& it, _Args&& ... arguments)
			{
				size_type index = it.ptr - _data;
				emplace(index, std::forward<_Args>(arguments)...);
			}

			void resize(size_type newSize)
			{
				if (newSize <= _size)
				{
					deleteRange(_data + newSize, _data + _size);
					_size = newSize;
				}
				else if (newSize <= _capacity)
				{
					constructRange(_data + _size, _data + newSize);
					_size = newSize;
				}
				else
				{
					size_type newCapacity = newSize * mulCapacity + addCapacity;
					if(newCapacity > _capacity)
						reallocate(newCapacity);
					
					constructRange(_data + _size, _data + newSize);
					_size = newSize;
				}
			}

			void resize(size_type newSize, const T& fillWith)
			{
				if (newSize <= _size)
				{
					deleteRange(_data + newSize, _data + _size);
					_size = newSize;
				}
				else if (newSize <= _capacity)
				{
					constructRange(_data + _size, _data + newSize, fillWith);
					_size = newSize;
				}
				else
				{
					size_type newCapacity = newSize * mulCapacity + addCapacity;
					if(newCapacity > _capacity)
						reallocate(newCapacity);
					
					constructRange(_data + _size, _data + newSize, fillWith);
					for (size_type i = _size; i < newSize; i++)
						::new((void*)(_data + i)) T(fillWith);

					_size = newSize;
				}
			}
			
			void resize_no_construct(size_type newSize)
			{
				if (newSize <= _size)
				{
					deleteRange(_data + newSize, _data + _size);
					_size = newSize;
				}
				else if (newSize <= _capacity)
				{
					_size = newSize;
				}
				else
				{
					size_type newCapacity = newSize;
					if(newCapacity > _capacity)
						reallocate(newCapacity);
					
					_size = newSize;
				}
			}
			
			T& operator[](size_type index)
			{
				return _data[index];
			}

			const T& operator[](size_type index) const
			{
				return _data[index];
			}

			iterator begin()
			{
				return iterator(_data);
			}

			iterator end()
			{
				return iterator(_data + _size);
			}

			const_iterator begin() const
			{
				return const_iterator(_data);
			}

			const_iterator end() const
			{
				return const_iterator(_data + _size);
			}

			T* front() const
			{
				return _data;
			}

			T* back() const
			{
				return (_data + _size) - 1;
			}

			size_type size() const
			{
				return _size;
			}

			size_type capacity() const
			{
				return _capacity;
			}

			T* data() const
			{
				return _data;
			}
		};
	}
}