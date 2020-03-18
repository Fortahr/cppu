#pragma once

#include "../dtypes.h"
#include <bitset>
#include <queue>
#include <unordered_map>

#include "pointers.h"
#include "details/key_counter.h"
#include "details/icontainer.h"
#include "details/garbage_cleaner.h"
#include "constructor.h"

#include "../bitops.h"

#ifndef WIN32
#define __forceinline __attribute__((always_inline))
#endif

// Container Garbage Collection

namespace cppu
{
	namespace cgc
	{
		namespace details
		{
			template<class K, class V>
			class base_unordered_map
			{
			protected:
				std::unordered_map<K, V> slots;
				std::queue<K> garbage;

				__forceinline V& base_slots(const K& key)
				{
					return reinterpret_cast<std::unordered_map<K, V>&>(slots)[key];
				}

				template<class... _Args>
				__forceinline V* base_emplace(const K& key, _Args&&... arguments)
				{
					// Construct object
					void* slot = &slots[key];
					constructor::construct_object<V>(slot, std::forward<_Args>(arguments)...);
					return static_cast<V*>(slot);
				}

				__forceinline void base_add_as_garbage(void* ptr, const base_counter* c)
				{
					garbage.push(static_cast<const key_counter<K>*>(c)->get_key());
				}

				__forceinline uint base_clean_garbage(uint max = std::numeric_limits<uint>::max())
				{
					uint i = 0;
					while (!garbage.empty() && i < max)
					{
						K key = garbage.front();
						garbage.pop();

						auto found = slots.find(key);
						if (found != slots.end())
						{
							// explicit deconstruction, otherwise it will never be deconstructed
							reinterpret_cast<V&>(found->second).~V();
							slots.erase(found);
						}

						++i;
					}

					return i;
				}

				__forceinline size_t base_size()
				{
					return slots.size();
				}

				__forceinline V& base_front()
				{
					return slots.front();
				}

				__forceinline V& base_back()
				{
					return slots.back();
				}

				__forceinline typename std::unordered_map<K, V>::iterator base_erase(const K& key)
				{
					return slots.erase(key);
				}

				__forceinline typename std::unordered_map<K, V>::iterator base_find(const K& obj)
				{
					return slots.find(obj);
				}

				__forceinline typename std::unordered_map<K, V>::iterator base_begin()
				{
					return slots.begin();
				}

				__forceinline typename std::unordered_map<K, V>::iterator base_end()
				{
					return slots.end();
				}

				__forceinline bool base_exists(K& key)
				{
					return slots.count(key);
				}

				__forceinline std::size_t base_garbage_size()
				{
					return garbage.size();
				}

				__forceinline bool base_garbage_empty()
				{
					return garbage.empty();
				}
			};
		}

		template<class K, class V, bool auto_clean = true>
		class unordered_map : private details::base_unordered_map<K, V>, public details::icontainer
		{
		private:
			typedef details::base_unordered_map<K, V> BASE_MAP;

			template<bool M = auto_clean, typename std::enable_if<!M>::type* = nullptr>
			inline void auto_garbage_clean(void* ptr, const details::base_counter* c)
			{ }

			template<bool M = auto_clean, typename std::enable_if<M>::type* = nullptr>
			inline void auto_garbage_clean(void* ptr, const details::base_counter* c)
			{
				if (BASE_MAP::garbage.size() == 1)
					details::garbage_cleaner::add_to_clean(this);
			}

		public:
			unordered_map()
			{ }

			// This one shouldn't be called unless all of the members are not being used anymore anywhere.
			~unordered_map()
			{ }

			template<class... _Args>
			inline strong_ptr<V> emplace(const K& key, _Args&&... arguments)
			{
				V* object = BASE_MAP::base_emplace(key, std::forward<_Args>(arguments)...);
				return constructor::construct_pointer(object, new details::key_counter<K>(this, key, [](const void* x) { static_cast<const V*>(x)->~V(); }));
			}

			virtual void add_as_garbage(void* ptr, const details::base_counter* c)
			{
				BASE_MAP::base_add_as_garbage(ptr, c);
				auto_garbage_clean(ptr, c);
			}

			virtual uint clean_garbage(uint max = std::numeric_limits<uint>::max())
			{
				return BASE_MAP::base_clean_garbage(max);
			}

			inline size_t size()
			{
				return BASE_MAP::base_size();
			}

			inline V& operator[] (K key)
			{
				return BASE_MAP::base_slots(key);
			}

			inline V& front()
			{
				return BASE_MAP::base_front();
			}

			inline V& back()
			{
				return BASE_MAP::base_back();
			}

			inline typename std::unordered_map<K, V>::iterator erase(const K& key)
			{
				return BASE_MAP::base_erase(key);
			}

			inline typename std::unordered_map<K, V>::iterator find(const K& obj)
			{
				return BASE_MAP::base_find(obj);
			}

			inline typename std::unordered_map<K, V>::iterator begin()
			{
				return BASE_MAP::base_begin();
			}

			inline typename std::unordered_map<K, V>::iterator end()
			{
				return BASE_MAP::base_end();
			}

			inline bool exists(K& key)
			{
				return BASE_MAP::exists(key);
			}

			inline std::size_t garbage_size()
			{
				return BASE_MAP::garbage_size();
			}

			inline bool garbage_empty()
			{
				return BASE_MAP::garbage_empty();
			}
		};
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif