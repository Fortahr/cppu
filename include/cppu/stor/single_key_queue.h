#pragma once

#include <mutex>
#include <unordered_set>
#include <queue>
#include <algorithm>

namespace cppu
{
	namespace stor
	{
		template<typename K, typename T, class _Hasher = std::hash<K>>
		class single_key_queue
		{
		public:
			struct QueueValue
			{
				K key;
				T value;

				QueueValue(K key, T value)
					: key(key)
					, value(value)
				{ }

				template<class... _Valty>
				QueueValue(K key, _Valty&&... values)
					: key(key)
					, value(std::forward<_Valty>(values)...)
				{ }
			};

		private:
			std::unordered_set<K, _Hasher> keys;
			std::queue<QueueValue> container;

		public:
			single_key_queue()
			{}

			single_key_queue(const single_key_queue& copy)
				: container(copy.container)
				, keys(copy.keys)
			{ }

			single_key_queue& operator=(const single_key_queue& copy)
			{
				container = copy.container;
				keys = copy.keys;

				return *this;
			}

			single_key_queue(single_key_queue&& move)
				: container(std::move(move.container))
				, keys(std::move(move.keys))
			{ }

			single_key_queue& operator=(single_key_queue&& move)
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
				std::queue<QueueValue>().swap(container);
				keys.clear();
			}

			template<class... _Valty>
			inline void emplace(const K& key, _Valty&&... values)
			{
				if (keys.emplace(key).second)
					container.emplace(key, std::forward<_Valty>(values)...);
			}

			inline void push(const K& key, const T& value)
			{
				if (keys.emplace(key).second)
					container.emplace(key, value);
			}

			bool pop(T& value)
			{
				if (!container.empty())
				{
					QueueValue contents = container.front();
					container.pop();

					keys.erase(contents.key);
					value = contents.value;

					return true;
				}

				return false;
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

			inline bool contains(const K& key)
			{
				return keys.contains(key);
			}

			inline void swapQueue(std::queue<QueueValue>& queue)
			{
				std::swap(container, queue);
			}

			inline void swapKeys(std::unordered_set<K, _Hasher>& keys)
			{
				std::swap(this->keys, keys);
			}

			inline void swap(std::unordered_set<K, _Hasher>& keys, std::queue<QueueValue>& queue)
			{
				std::swap(container, queue);
				std::swap(this->keys, keys);
			}
		};
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif