#pragma once

#include <atomic>
#include <bitset>
#include <queue>
#include <limits>
#include <cassert>

#include "pointers.h"
#include "details/container_counter.h"
#include "details/icontainer.h"
#include "details/garbage_cleaner.h"
#include "constructor.h"

#include "../bitops.h"

// Container Garbage Collection

namespace cppu
{
	namespace cgc
	{
		typedef uint8_t SIZE_8;
		typedef uint16_t SIZE_16;
		typedef uint32_t SIZE_32;
		typedef uint64_t SIZE_64;
		
		template<class T, class S = SIZE_32, CLEAN_PROC clean_proc = CLEAN_PROC::DIRECT>
		class array : public details::icontainer
		{
			template<typename, typename, CLEAN_PROC> friend class m_array;
		public:
			static constexpr size_t size() { return std::numeric_limits<S>::digits; }

		private:
			static constexpr size_t const npos = std::numeric_limits<S>::digits;

			// Disable RAII by using a union as this should only reserve/allocate memory for the types later on
			union
			{
				T slots[size()];
			};

			std::atomic<S> freeSlots;
			std::atomic<S> initSlots;
			std::atomic<S> garbage;

			inline static size_t first_free(const S& freeSlots)
			{
				return bs_rtol(freeSlots); // bit scan
			}

			inline size_t reserve_spot()
			{
				// Find first available spot, in a thread safe & lock free approach
				S freeSlotsCheck = freeSlots.load();
				S freeSlotsCopy = freeSlotsCheck;
				size_t freeSpot;

				while ((freeSpot = first_free(freeSlotsCheck)) < npos)
				{
					S newValue = freeSlotsCheck & ~(1 << freeSpot);
					if (!freeSlots.compare_exchange_weak(freeSlotsCheck, newValue))
					{
						// CAS weak can fail on non x86 chipsets, 
						// memory can be set with the new value anyhow (success), so check the value
						if (freeSlotsCheck != freeSlotsCopy)
						{
							freeSlotsCopy = freeSlotsCheck;
							continue;
						}
					}

					return freeSpot;
				}

				return npos;
			}

			template<class... _Args>
			inline strong_ptr<T> emplace_at(size_t slot, _Args&&... arguments)
			{
				assert(slot < npos);

				// Construct object
				T* object = slots + slot;
				constructor::construct_object<T>(object, std::forward<_Args>(arguments)...);
				
				// note the
				S oldValue, newValue;
				do
				{
					oldValue = initSlots;
					newValue = oldValue | (1 << slot);
				} while (!initSlots.compare_exchange_weak(oldValue, newValue));

				return constructor::construct_pointer(object, new details::container_counter(this, details::destructor::create_destructor<T>()));
			}

			inline void clean_and_destruct_slot(size_t offset)
			{
				S oldValue, newValue;
				T& item = slots[offset];
				item.~T();

				// reset slot value as uninitialized
				do
				{
					oldValue = initSlots;
					newValue = oldValue & ~(1 << offset);
				} while (!initSlots.compare_exchange_weak(oldValue, newValue));

				// reset slot value as free
				do
				{
					oldValue = freeSlots;
					newValue = oldValue ^ (1 << offset);
				} while (!freeSlots.compare_exchange_weak(oldValue, newValue));
			}

		public:
			struct iterator
			{
				template<class, class, CLEAN_PROC> friend class cgc::array;
				friend struct cgc::m_array<T, S, clean_proc>::iterator;

				using iterator_category = std::random_access_iterator_tag;
				using difference_type = std::ptrdiff_t;
				using value_type = T;
				using pointer = T*;
				using reference = T&;

			private:
				S offset;
				cgc::array<T, S, clean_proc>* arr;

				iterator(cgc::array<T, S, clean_proc>* arr, S offset)
					: arr(arr)
					, offset(offset)
				{ }

			public:
				iterator(const iterator& it)
					: arr(it.arr)
					, offset(it.offset)
				{ }

				iterator operator+(size_t offset) const
				{
					return { arr, arr->next_index(this->offset + offset) };
				}

				iterator& operator+=(size_t offset)
				{
					this->offset = arr->next_index(this->offset + offset);
					return *this;
				}

				iterator operator++()
				{
					this->offset = arr->next_index(this->offset + 1);
					return *this;
				}

				iterator operator++(int)
				{
					S offs = this->offset;
					this->offset = arr->next_index(this->offset + 1);
					return iterator(arr, offs);
				}

				iterator operator-(size_t offset) const
				{
					return { arr, arr->prev_index(this->offset - offset) };
				}

				iterator& operator-=(size_t offset)
				{
					this->offset = arr->prev_index(this->offset - offset);
					return *this;
				}

				iterator& operator--()
				{
					this->offset = arr->next_index(this->offset - 1);
					return *this;
				}

				iterator operator--(int)
				{
					S offs = offset;
					this->offset = arr->next_index(this->offset - 1);
					return iterator(arr, offs);
				}

				bool operator==(const iterator& other) const
				{
					return offset == other.offset;
				}

				bool operator!=(const iterator& other) const
				{
					return offset != other.offset;
				}

				T* operator->() const
				{
					return arr->slots + offset;
				}

				T& operator*() const
				{
					return arr->slots[offset];
				}
			};

			array()
				: freeSlots(~0)
				, initSlots(0)
				, garbage(0)
			{ }

			// This one shouldn't be called unless all of the members are not being used anymore anywhere.
			~array()
			{
				S takenSpots = ~freeSlots;
				for (S i = bs_rtol(takenSpots); i < npos; i = bs_rtol(takenSpots, ++i))
					slots[i].~T();
			}

			template<class... _Args>
			strong_ptr<T> emplace(_Args&&... arguments)
			{
				strong_ptr<T> pointer;

				// Find first available spot, in a thread safe & lock free approach
				S freeSpot = reserve_spot();
				if (freeSpot < npos)
					pointer = emplace_at(freeSpot, std::forward<_Args>(arguments)...);

				// Return object
				return pointer;
			}

			bool add_as_garbage(void* ptr, const details::base_counter* c) override
			{
				if constexpr (clean_proc == CLEAN_PROC::DIRECT)
					clean_and_destruct_slot((reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(slots)) / sizeof(T));
				else
				{
					size_t offset = (reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(slots)) / sizeof(T);
					S oldValue, newValue;
					do
					{
						oldValue = garbage;
						newValue = oldValue | (1 << offset);
					} while (!garbage.compare_exchange_weak(oldValue, newValue));

					if constexpr (clean_proc == CLEAN_PROC::THREAD)
					{
						if (oldValue == 0)
							details::garbage_cleaner::add_to_clean(this);
					}
				}

				return true;
			}

			size_t clean_garbage(size_t max = std::numeric_limits<size_t>::max()) override
			{
				if constexpr (clean_proc == CLEAN_PROC::MANUAL)
				{
					size_t i = 0;
					for (size_t offset = bs_rtol(garbage); offset < npos; offset = bs_rtol(garbage, offset))
					{
						clean_and_destruct_slot(offset);

						S oldValue, newValue;
						// set garbage slot value as free
						do
						{
							oldValue = garbage;
							newValue = oldValue & ~(1 << offset);
						} while (!garbage.compare_exchange_weak(oldValue, newValue));

						++i;
					}

					return i;
				}
				else
					return 0;
			}

			inline T& operator[] (size_t pos)
			{
				return slots[pos];
			}

			inline T& at(size_t pos)
			{
				if (pos < size())
					return slots[pos];
				else
					throw std::out_of_range();
			}

			inline T& front()
			{
				return slots[bs_rtol(initSlots.load())];
			}

			inline T& back()
			{
				return slots[bs_ltor(initSlots.load())];
			}

			inline T& next(S pos)
			{
				return slots[next_index(pos)];
			}

			inline T& prev(S pos)
			{
				return slots[prev_index(pos)];
			}

			inline S front_index()
			{
				return bs_rtol(initSlots.load());
			}

			inline S back_index()
			{
				return bs_ltor(initSlots.load());
			}

			inline S next_index(S pos)
			{
				return bs_rtol(initSlots.load(), pos);
			}

			inline S prev_index(S pos)
			{
				return bs_ltor(initSlots.load(), pos);
			}

			iterator begin()
			{
				return iterator(this, bs_rtol(initSlots));
			}

			iterator end()
			{
				return iterator(this, npos);
			}

			bool exists(T& object)
			{
				S takenSpots = ~freeSlots;
				for (size_t i = bs_rtol(takenSpots); i < npos; i = bs_rtol(takenSpots, i))
				{
					if (slots[i] == object)
						return true;
				}

				return false;
			}
			
			bool exists(T* object)
			{
				// to be implemented
			}

			size_t indexof(T& object)
			{
				S takenSpots = ~freeSlots;
				for (size_t i = bs_rtol(freeSlots); i < npos; i = bs_rtol(freeSlots, i))
				{
					for (size_t s = i; s < i; ++s)
					{
						if (slots[s] == object)
							return s;
					}
				}

				return npos;
			}

			size_t indexof(T* object)
			{
				size_t index = (static_cast<uintptr_t>(object) - static_cast<uintptr_t>(slots)) / sizeof(T);
				assert(index >= 0 && index < npos);
				return index;
			}

			inline size_t garbage_size()
			{
				return garbage.size();
			}

			inline bool garbage_empty()
			{
				return garbage.empty();
			}
		};
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif