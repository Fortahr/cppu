#pragma once

#include "../dtypes.h"
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
		typedef uint8 SIZE_8;
		typedef uint16 SIZE_16;
		typedef uint32 SIZE_32;
		typedef uint64 SIZE_64;
		
		template<class T, class S = SIZE_32, bool auto_clean = true>
		class array : public details::icontainer
		{
			template<typename, typename, bool> friend class m_array;
		public:
			static constexpr uint size() { return std::numeric_limits<S>::digits; }

		private:
			static constexpr uint const npos = std::numeric_limits<S>::digits;

			byte slots[size() * sizeof(T)];
			std::atomic<S> freeSlots;
			std::atomic<S> initSlots;
			std::atomic<S> garbage;

			template<bool A = auto_clean, typename std::enable_if<!A>::type* = nullptr>
			inline void auto_garbage_clean(void* ptr, const details::base_counter* c)
			{ }

			template<bool A = auto_clean, typename std::enable_if<A>::type* = nullptr>
			inline void auto_garbage_clean(void* ptr, const details::base_counter* c)
			{
				details::garbage_cleaner::add_to_clean(this);
			}

			inline T* Slots(uint i)
			{
				return reinterpret_cast<T*>(&slots) + i;
			}

			inline static uint first_free(const S& freeSlots)
			{
				return bs_rtol(freeSlots); // bit scan
			}

			inline uint reserve_spot()
			{
				// Find first available spot, in a thread safe & lock free approach
				S freeSlotsCheck = freeSlots.load();
				S freeSlotsCopy = freeSlotsCheck;
				uint freeSpot;

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
			inline strong_ptr<T> emplace_at(uint slot, _Args&&... arguments)
			{
				assert(slot < npos);

				// Construct object
				T* object = Slots(slot);
				constructor::construct_object<T>(object, std::forward<_Args>(arguments)...);
				
				// note the
				S oldValue, newValue;
				do
				{
					oldValue = initSlots;
					newValue = oldValue | (1 << slot);
				} while (!initSlots.compare_exchange_weak(oldValue, newValue));

				return constructor::construct_pointer(object, new details::container_counter(this, [](const void* x) { static_cast<const T*>(x)->~T(); }));
			}

		public:
			array()
				: freeSlots(~0)
				, initSlots(0)
				, garbage()
			{

			}

			// This one shouldn't be called unless all of the members are not being used anymore anywhere.
			~array()
			{
				S takenSpots = ~freeSlots;
				for (S i = bs_rtol(takenSpots); i < npos; i = bs_rtol(takenSpots, ++i))
					Slots(i)->~T();
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

			virtual void add_as_garbage(void* ptr, const details::base_counter* c)
			{
				uint offset = (reinterpret_cast<std::size_t>(ptr) - reinterpret_cast<std::size_t>(slots)) / sizeof(T);
				S oldValue, newValue;
				do
				{
					oldValue = garbage;
					newValue = oldValue | (1 << offset);
				} while (!garbage.compare_exchange_weak(oldValue, newValue));

				if(oldValue == 0)
					auto_garbage_clean(ptr, c);
			}


			uint clean_garbage(uint max = std::numeric_limits<uint>::max())
			{
				uint i = 0;
				for (uint offset = bs_rtol(garbage); offset < npos; offset = bs_rtol(garbage, offset))
				{
					S oldValue, newValue;
					T* item = Slots(offset);
					item->~T();

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

			inline T* operator[] (uint i)
			{
				return Slots(i);
			}

			inline uint front()
			{
				S takenSpots = initSlots;
				return bs_rtol(takenSpots);
			}

			inline T* back(uint& i)
			{
				S takenSpots = initSlots;
				i = bs_ltor(takenSpots);
				return Slots(i);
			}

			inline uint next(uint i)
			{
				S takenSpots = initSlots;
				return bs_rtol(takenSpots, i);
			}

			inline T* prev(uint& i)
			{
				S takenSpots = initSlots;
				i = bs_ltor(takenSpots, i);
				return Slots(i);
			}

			class iterator
			{
			private:
				S offset;
				cgc::array<T, S>* arr;

				iterator(cgc::array<T, S>* arr, S offset)
					: arr(arr)
					, offset(offset)
				{

				}

			public:
				iterator& operator + (int offset)
				{
					this->offset = arr->next(this->offset + offset);
					return *this;
				}

				T* operator -> () const
				{
					return arr->Slots(offset);
				}

				bool operator == (iterator& other)
				{
					return this->offset == other->offset;
				}
			};

			iterator begin()
			{
				return iterator(front(), this);
			}

			iterator end()
			{
				return iterator(npos, this);
			}

			bool exists(T* object)
			{
				S takenSpots = ~freeSlots;
				for (uint i = bs_rtol(takenSpots); i < npos; i = bs_rtol(takenSpots, i))
				{
					if (Slots(i) == object)
						return true;
				}

				return false;
			}

			uint indexof(T* object)
			{
				S takenSpots = ~freeSlots;
				for (uint i = bs_rtol(freeSlots); i < npos; i = bs_rtol(freeSlots, i))
				{
					for (uint s = i; s < i; ++s)
					{
						if (Slots(s) == object)
							return s;
					}
				}

				return npos;
			}

			inline std::size_t garbage_size()
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