#pragma once

#include <mutex>
#include <unordered_set>
#include <vector>
#include <algorithm>

namespace cppu
{
	namespace stor
	{
		template<typename K, typename T, class _Hasher = std::hash<K>>
		class single_key_vector
		{
		public:
			struct VectorValue
			{
				K key;
				T value;

				VectorValue(K key, T value)
					: key(key)
					, value(value)
				{ }

				template<class... _Valty>
				VectorValue(K key, _Valty&&... values)
					: key(key)
					, value(std::forward<_Valty>(values)...)
				{ }
			};

		private:
			std::unordered_set<K, _Hasher> keys;
			std::vector<VectorValue> container;

		public:
			single_key_vector()
			{}

			single_key_vector(const single_key_vector& copy)
				: container(copy.container)
				, keys(copy.keys)
			{ }

			single_key_vector& operator=(const single_key_vector& copy)
			{
				container = copy.container;
				keys = copy.keys;

				return *this;
			}

			single_key_vector(single_key_vector&& move)
				: container(std::move(move.container))
				, keys(std::move(move.keys))
			{ }

			single_key_vector& operator=(single_key_vector&& move)
			{
				container = std::move(move.container);
				keys = std::move(move.keys);

				return *this;
			}
			/*inline typename std::deque<T>::iterator begin()
			{
				return container.begin();
			}

			inline typename std::deque<T>::const_iterator begin() const
			{
				return container.begin();
			}

			inline typename std::deque<T>::iterator end()
			{
				return container.end();
			}

			inline typename std::deque<T>::const_iterator end() const
			{
				return container.end();
			}*/

			inline bool empty() const
			{
				return container.empty();
			}

			inline size_t size() const
			{
				return container.size();
			}

			inline void clear()
			{
				std::queue<VectorValue>().swap(container);
				keys.clear();
			}

			template<class... _Valty>
			inline K* emplace_front(const K& key, _Valty&&... values)
			{
				if (keys.emplace(key).second)
					return &container.emplace_front(key, std::forward<_Valty>(values)...);
				
				return nullptr;
			}

			template<class... _Valty>
			inline K* emplace_back(const K& key, _Valty&&... values)
			{
				if (keys.emplace(key).second)
					return &container.emplace_back(key, std::forward<_Valty>(values)...);

				return nullptr;
			}

			inline typename std::vector<VectorValue>::iterator erase(typename std::vector<VectorValue>::const_iterator it)
			{
				keys.erase(it->first);				
				return container.erase(it);
			}

			inline typename std::vector<VectorValue>::iterator erase(typename std::vector<VectorValue>::const_iterator from, typename std::vector<VectorValue>::const_iterator last)
			{
				for(auto it = from; it != last; ++it)
					keys.erase(it->first);

				return container.erase(from, last);
			}

			/*T& front()
			{
				//std::unique_lock<std::mutex> lk(lock);
				return container.front();
			}

			T& back()
			{
				//std::unique_lock<std::mutex> lk(lock);
				return container.back();
			}*/

			inline VectorValue& front() noexcept
			{
				return container.front();
			}

			inline VectorValue& back() noexcept
			{
				return container.back();
			}
			
			inline typename std::vector<VectorValue>::iterator begin() noexcept
			{
				return container.begin();
			}

			inline typename std::vector<VectorValue>::const_iterator begin() const noexcept
			{
				return container.begin();
			}

			inline typename std::vector<VectorValue>::iterator end() noexcept
			{
				return container.end();
			}

			inline typename std::vector<VectorValue>::const_iterator end() const noexcept
			{
				return container.end();
			}

			inline bool contains(const K& key) const
			{
				return keys.contains(key);
			}

			inline void swapVector(std::vector<VectorValue>& vector) noexcept
			{
				std::swap(container, vector);
			}

			inline void swapKeys(std::unordered_set<K, _Hasher>& keys) noexcept
			{
				std::swap(this->keys, keys);
			}

			inline void swap(std::unordered_set<K, _Hasher>& keys, std::vector<VectorValue>& vector) noexcept
			{
				std::swap(container, vector);
				std::swap(this->keys, keys);
			}
		};
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif