#pragma once

#include <mutex>
#include <queue>
#include <algorithm>

namespace cppu
{
	namespace stor
	{
		namespace lock
		{
			template<typename T>
			class queue
			{
			private:
				std::queue<T> container;
				std::mutex lock;

			public:
				queue()
				{}

				queue(queue& other)
					: container(other.container)
				{}

				queue& operator=(const queue& other)
				{
					container = other.container;
					return *this;
				}

				/*T& operator[](std::size_t i)
				{
					return container[i];
				}

				inline T& at(std::size_t i)
				{
					return container[i];
				}

				inline const T& at(std::size_t i) const
				{
					return container[i];
				}

				inline typename std::queue<T>::iterator begin()
				{
					return container.begin();
				}

				inline typename std::queue<T>::const_iterator begin() const
				{
					return container.begin();
				}

				inline typename std::queue<T>::iterator end()
				{
					return container.end();
				}

				inline typename std::queue<T>::const_iterator end() const
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
					std::lock_guard<std::mutex> lk(lock);
					container.clear();
				}

				/*inline typename std::queue<T>::iterator erase(const typename std::queue<T>::const_iterator& it)
				{
					std::lock_guard<std::mutex> lk(lock);
					return container.erase(it);
				}

				inline typename std::queue<T>::iterator erase(typename std::queue<T>::iterator& obj)
				{
					std::lock_guard<std::mutex> lk(lock);
					return container.erase(obj);
				}
				
				void insert(const typename std::queue<T>::const_iterator& insert_from, const typename std::queue<T>::const_iterator& from, const typename std::queue<T>::const_iterator& to)
				{
					std::lock_guard<std::mutex> lk(lock);
					container.insert(insert_from, from, to);
				}*/

				template<class... _Valty>
				void emplace(_Valty&&... _Val)
				{
					std::lock_guard<std::mutex> lk(lock);
					container.emplace(std::forward<_Valty>(_Val)...);
				}

				void push(const T& obj)
				{
					std::lock_guard<std::mutex> lk(lock);
					container.push(obj);
				}

				void push(T&& obj)
				{
					std::lock_guard<std::mutex> lk(lock);
					container.push(std::move(obj));
				}


				void pop()
				{
					std::lock_guard<std::mutex> lk(lock);
					container.pop();
				}

				bool pop(T& obj)
				{
					std::lock_guard<std::mutex> lk(lock);
					if (!container.empty())
					{
						obj = std::move(container.back());
						container.pop_back();

						return true;
					}
					else
						return false;
				}

				T& front()
				{
					//std::unique_lock<std::mutex> lk(lock);
					return container.front();
				}

				std::queue<T>& get_queue_and_lock(std::unique_lock<std::mutex>& lk)
				{
					lk = std::unique_lock<std::mutex>(lock);
					return container;
				}

				const std::queue<T>& get_queue_and_lock(std::unique_lock<std::mutex>& lk) const
				{
					lk = std::unique_lock<std::mutex>(const_cast<std::mutex&>(lock));
					return container;
				}

				inline void swap(std::queue<T>& swap)
				{
					std::lock_guard<std::mutex> lk(lock);
					this->container.swap(swap);
				}

				inline void swap(queue<T>& swap)
				{
					std::lock_guard<std::mutex> lk(lock);
					this->container.swap(swap.container);
				}
			};
		}
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif