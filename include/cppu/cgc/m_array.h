#pragma once

#include <mutex>

#include "../ncm.h"
#include "array.h"

namespace cppu
{
	namespace cgc
	{
		// Don't destruct this container when any thread is still accessing it, e.g: emplace()
		template<class T, class S = SIZE_32, CLEAN_PROC clean_proc = CLEAN_PROC::DIRECT>
		class m_array
		{
		private:
			std::vector<cgc::array<T, S, clean_proc>*> arrays;
			ncm<std::mutex> lock;

		public:
			struct iterator
			{
				template<class, class, CLEAN_PROC> friend class cgc::m_array;

				using iterator_category = std::random_access_iterator_tag;
				using difference_type = std::ptrdiff_t;
				using value_type = T;
				using pointer = T*;
				using reference = T&;

			private:
				cgc::m_array<T, S, clean_proc>& m_arr;
				typename cgc::array<T, S, clean_proc>::iterator it;

				iterator(cgc::m_array<T, S, clean_proc>& m_arr, typename cgc::array<T, S, clean_proc>::iterator it)
					: m_arr(m_arr)
					, it(it)
				{ }

			public:
				iterator(const iterator& it)
					: m_arr(it.m_arr)
					, it(it.it)
				{ }

				/*iterator operator+(size_t offset)
				{
					return { m_arr, it + offset };
				}

				iterator& operator+=(size_t offset)
				{
					it += offset;
					return *this;
				}*/

				iterator& operator++()
				{
					++it;

					while (it.offset == cgc::array<T, S, clean_proc>::npos && it.arr != m_arr.arrays.back())
					{
						++it.arr;
						it.offset = it.arr->front_index();
					}

					return *this;
				}

				iterator operator++(int)
				{
					auto cpy = it;
					this->operator++();
					return { m_arr, cpy };
				}

				/*iterator operator-(size_t offset)
				{
					return { m_arr, it - offset };
				}

				iterator& operator-=(size_t offset)
				{
					it -= offset;
					return *this;
				}*/

				iterator& operator--()
				{
					--it;

					while (it.offset == cgc::array<T, S, clean_proc>::npos && it.arr != m_arr.arrays.front())
					{
						--it.arr;
						it.offset = it.arr->back_index();
					}

					return *this;
				}

				iterator operator--(int)
				{
					auto cpy = it;
					this->operator--();
					return { m_arr, cpy };
				}

				bool operator==(const iterator& other)
				{
					return it == other.it;
				}

				bool operator!=(const iterator& other)
				{
					return it != other.it;
				}

				T* operator->()
				{
					return it.operator->();
				}

				T& operator*() const
				{
					return it.operator*();
				}
			};

			m_array()
			{
				cgc::array<T, S, clean_proc>* arr = new cgc::array<T, S, clean_proc>();
				arrays.push_back(arr);
			}

			~m_array()
			{
				lock.lock();

				for (uint i = 0; i < arrays.size(); ++i)
				{
					cgc::array<T, S, clean_proc>* arr = arrays[i];
					delete arr;
				}

				lock.unlock();
			}

			template<class... _Args>
			strong_ptr<T> emplace(_Args&&... arguments)
			{
				cgc::array<T, S, clean_proc>* container = nullptr;
				uint i = 0;

			RETRY_EMPLACE:
				std::size_t size = arrays.size();
				for (; i < size; ++i)
				{
					uint slot = arrays[i]->reserve_spot();
					if (slot != cgc::array<T, S, clean_proc>::npos)
						return arrays[i]->emplace_at(slot, std::forward<_Args>(arguments)...);
				}

				lock.lock();

				// another thread might've created a new array already, retry
				if (size < arrays.size())
				{
					lock.unlock();
					goto RETRY_EMPLACE;
				}

				// no suitable spot found in current arrays, build a new one
				container = new cgc::array<T, S, clean_proc>();
				arrays.push_back(container);

				lock.unlock();

				// Return object
				return container->emplace(std::forward<_Args>(arguments)...);
			}

			inline std::mutex& get_lock()
			{
				return lock;
			}

			inline std::vector<cgc::array<T, S, clean_proc>*>& get_arrays()
			{
				return arrays;
			}

			virtual uint clean_garbage(uint max = std::numeric_limits<uint>::max())
			{
				uint newMax = max;
				if constexpr (clean_proc != CLEAN_PROC::DIRECT)
				{
					for (std::size_t i = 0; i < arrays.size(); ++i)
						newMax -= arrays[i]->clean_garbage(newMax);
				}

				return max - newMax;
			}

			inline std::size_t garbage_size()
			{
				uint size = 0;
				if constexpr (clean_proc != CLEAN_PROC::DIRECT)
				{
					for (std::size_t i = 0; i < arrays.size(); ++i)
						size += arrays[i]->garbage_size();
				}

				return size;
			}

			inline bool garbage_empty()
			{
				if constexpr (clean_proc != CLEAN_PROC::DIRECT)
				{
					for (std::size_t i = 0; i < arrays.size(); ++i)
					{
						if (!arrays[i]->garbage_empty())
							return false;
					}
				}

				return true;
			}

			inline iterator begin()
			{
				return { *this, arrays.front()->begin() };
			}

			inline iterator end()
			{
				return { *this, arrays.back()->end() };
			}
		};
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif