#pragma once

#include "../unordered_map.h"
#include "../details/counter_value_pair.h"

// Container Garbage Collection

namespace cppu
{
	namespace cgc
	{
		namespace ref
		{
			template<class K, class V, bool auto_clean = true>
			class unordered_map : private details::base_unordered_map<K, details::counter_value_pair<V>>, public details::icontainer
			{
			private:
				typedef details::counter_value_pair<V> VALUE;
				typedef details::base_unordered_map<K, VALUE> BASE_MAP;
				typedef std::unordered_map<K, VALUE> INTERNAL_MAP;

				template<bool A = auto_clean, typename std::enable_if<!A>::type* = nullptr>
				inline void auto_garbage_clean(void* ptr, const details::base_counter* c)
				{ }

				template<bool A = auto_clean, typename std::enable_if<A>::type* = nullptr>
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
					VALUE* object = BASE_MAP::base_emplace(key, new details::key_counter<K>(this, key, [](const void* x) { static_cast<const V*>(x)->~V(); }), std::forward<_Args>(arguments)...);
					return constructor::construct_pointer(&object->get_value(), object->get_counter());
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

				inline cgc::strong_ptr<V> operator[] (K key)
				{
					VALUE& element = BASE_MAP::base_slots(key);
					return constructor::construct_pointer(&element->value, element->counter);
				}

				inline cgc::strong_ptr<V> front()
				{
					VALUE& element = BASE_MAP::base_front();
					return constructor::construct_pointer(&element->value, element->counter);
				}

				inline cgc::strong_ptr<V> back()
				{
					VALUE& element = BASE_MAP::base_back();
					return constructor::construct_pointer(&element->value, element->counter);
				}

				inline typename std::unordered_map<K, VALUE>::iterator erase(const K& key)
				{
					return BASE_MAP::base_erase(key);
				}

				inline typename std::unordered_map<K, VALUE>::iterator find(const K& obj)
				{
					return BASE_MAP::base_find(obj);
				}

				inline typename std::unordered_map<K, VALUE>::iterator begin()
				{
					return BASE_MAP::base_begin();
				}

				inline typename std::unordered_map<K, VALUE>::iterator end()
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
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif