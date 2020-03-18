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
			class priority_queue
			{
			private:
				std::priority_queue<T> container;
				std::mutex lock;

			public:
				priority_queue()
				{}

				priority_queue(priority_queue& other)
					: container(other.container)
				{}

				priority_queue& operator=(priority_queue other)
				{
					container = other.container;
					return *this;
				}

				T& operator[](std::size_t i)
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

				inline typename std::deque<T>::iterator begin()
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
				}

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

				inline typename std::deque<T>::iterator erase(const typename std::deque<T>::const_iterator& it)
				{
					std::lock_guard<std::mutex> lk(lock);
					return container.erase(it);
				}

				inline typename std::deque<T>::iterator erase(typename std::deque<T>::iterator& obj)
				{
					std::lock_guard<std::mutex> lk(lock);
					return container.erase(obj);
				}

				void insert(const typename std::deque<T>::const_iterator& insert_from, const typename std::deque<T>::const_iterator& from, const typename std::deque<T>::const_iterator& to)
				{
					std::lock_guard<std::mutex> lk(lock);
					container.insert(insert_from, from, to);
				}

				template<class... _Valty>
				void emplace_back(_Valty&&... _Val)
				{
					std::lock_guard<std::mutex> lk(lock);
					container.emplace_back(std::forward<_Valty>(_Val)...);
				}

				template<class... _Valty>
				void emplace_front(_Valty&&... _Val)
				{
					std::lock_guard<std::mutex> lk(lock);
					container.emplace_front(std::forward<_Valty>(_Val)...);
				}

				void push_back(const T& obj)
				{
					std::lock_guard<std::mutex> lk(lock);
					container.push_back(obj);
				}

				void push_back(T&& obj)
				{
					std::lock_guard<std::mutex> lk(lock);
					container.push_back(std::move(obj));
				}

				bool push_back_if_not_exists(const T& obj)
				{
					std::lock_guard<std::mutex> lk(lock);
					bool notExists = std::find(container.begin(), container.end(), obj) == container.end();
					if (notExists)
						container.push_back(obj);

					return notExists;
				}

				bool push_back_if_not_exists(T& obj)
				{
					std::lock_guard<std::mutex> lk(lock);
					bool notExists = std::find(container.begin(), container.end(), obj) == container.end();
					if(notExists)
						container.push_back(obj);

					return notExists;
				}

				void push_front(const T& obj)
				{
					std::lock_guard<std::mutex> lk(lock);
					container.push_front(obj);
				}

				void push_front(T&& obj)
				{
					std::lock_guard<std::mutex> lk(lock);
					container.push_front(std::move(obj));
				}

				void pop_back()
				{
					std::lock_guard<std::mutex> lk(lock);
					container.pop_back();
				}

				void pop_front()
				{
					std::lock_guard<std::mutex> lk(lock);
					container.pop_front();
				}

				bool pop_back(T& obj)
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

				bool pop_front(T& obj)
				{
					std::lock_guard<std::mutex> lk(lock);
					if (!container.empty())
					{
						obj = std::move(container.front());
						container.pop_front();

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

				T& back()
				{
					//std::unique_lock<std::mutex> lk(lock);
					return container.back();
				}

				typename std::deque<T>::iterator find(const T& obj)
				{
					std::lock_guard<std::mutex> lk(lock);
					return std::find(container.begin(), container.end(), obj);
				}

				template<class _Pr1>
				typename std::deque<T>::iterator find_if(_Pr1 _Pred)
				{
					std::lock_guard<std::mutex> lk(lock);
					return std::find_if(container.begin(), container.end(), _Pred);
				}

				void remove(T& obj)
				{
					std::lock_guard<std::mutex> lk(lock);
					(void)std::remove(container.begin(), container.end(), obj);
				}

				template<class _Pr1>
				void remove_if(_Pr1 _Pred)
				{
					std::lock_guard<std::mutex> lk(lock);
					(void)std::remove_if(container.begin(), container.end(), _Pred);
				}

				template<class _Pr1>
				void sort(_Pr1 _Pred)
				{
					std::lock_guard<std::mutex> lk(lock);
					std::sort(container.begin(), container.end(), _Pred);
				}

				std::deque<T>& get_deque_and_lock(std::unique_lock<std::mutex>& lk)
				{
					lk = std::unique_lock<std::mutex>(lock);
					return container;
				}

				const std::deque<T>& get_deque_and_lock(std::unique_lock<std::mutex>& lk) const
				{
					lk = std::unique_lock<std::mutex>(const_cast<std::mutex&>(lock));
					return container;
				}

				inline void swap(std::deque<T>& swap)
				{
					std::lock_guard<std::mutex> lk(lock);
					this->container.swap(swap);
				}

				inline void swap(deque<T>& swap)
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