#pragma once

#include <shared_mutex>
#include <deque>
#include <algorithm>

namespace cppu
{
	namespace stor
	{
		namespace lock
		{
			template<typename T>
			class deque
			{
			private:
				std::deque<T> container;
				mutable std::shared_mutex lock;

			public:
				deque() = default;

				deque(deque& other)
					: container(other.container)
				{}

				deque& operator=(const deque& other)
				{
					container = other.container;
					return *this;
				}

				T& operator[](std::size_t i)
				{
					std::shared_lock lk(lock);
					return container[i];
				}

				inline T& at(std::size_t i)
				{
					std::shared_lock lk(lock);
					return container.at(i);
				}

				inline const T& at(std::size_t i) const
				{
					std::shared_lock lk(lock);
					return container.at(i);
				}

				inline typename std::deque<T>::iterator begin()
				{
					std::shared_lock lk(lock);
					return container.begin();
				}

				inline typename std::deque<T>::const_iterator begin() const
				{
					std::shared_lock lk(lock);
					return container.begin();
				}

				inline typename std::deque<T>::iterator end()
				{
					std::shared_lock lk(lock);
					return container.end();
				}

				inline typename std::deque<T>::const_iterator end() const
				{
					std::shared_lock lk(lock);
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
					std::unique_lock lk(lock);
					container.clear();
				}

				inline typename std::deque<T>::iterator erase(const typename std::deque<T>::const_iterator& it)
				{
					std::unique_lock lk(lock);
					return container.erase(it);
				}

				inline typename std::deque<T>::iterator erase(typename std::deque<T>::iterator& obj)
				{
					std::unique_lock lk(lock);
					return container.erase(obj);
				}

				void insert(const typename std::deque<T>::const_iterator& insert_from, const typename std::deque<T>::const_iterator& from, const typename std::deque<T>::const_iterator& to)
				{
					std::unique_lock lk(lock);
					container.insert(insert_from, from, to);
				}

				template<class... _Valty>
				void emplace_back(_Valty&&... _Val)
				{
					std::unique_lock lk(lock);
					container.emplace_back(std::forward<_Valty>(_Val)...);
				}

				template<class... _Valty>
				void emplace_front(_Valty&&... _Val)
				{
					std::unique_lock lk(lock);
					container.emplace_front(std::forward<_Valty>(_Val)...);
				}

				void push_back(const T& obj)
				{
					std::unique_lock lk(lock);
					container.push_back(obj);
				}

				void push_back(T&& obj)
				{
					std::unique_lock lk(lock);
					container.push_back(std::move(obj));
				}

				bool push_back_if_not_exists(const T& obj)
				{
					std::unique_lock lk(lock);
					bool notExists = std::find(container.begin(), container.end(), obj) == container.end();
					if (notExists)
						container.push_back(obj);

					return notExists;
				}

				bool push_back_if_not_exists(T& obj)
				{
					std::unique_lock lk(lock);
					bool notExists = std::find(container.begin(), container.end(), obj) == container.end();
					if(notExists)
						container.push_back(obj);

					return notExists;
				}

				void push_front(const T& obj)
				{
					std::unique_lock lk(lock);
					container.push_front(obj);
				}

				void push_front(T&& obj)
				{
					std::unique_lock lk(lock);
					container.push_front(std::move(obj));
				}

				void pop_back()
				{
					std::unique_lock lk(lock);
					container.pop_back();
				}

				void pop_front()
				{
					std::unique_lock lk(lock);
					container.pop_front();
				}

				bool pop_back(T& obj)
				{
					std::unique_lock lk(lock);
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
					std::unique_lock lk(lock);
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
					std::shared_lock lk(lock);
					return container.front();
				}

				T& back()
				{
					std::shared_lock lk(lock);
					return container.back();
				}

				typename std::deque<T>::iterator find(const T& obj)
				{
					std::shared_lock lk(lock);
					return std::find(container.begin(), container.end(), obj);
				}

				template<class _Pr1>
				typename std::deque<T>::iterator find_if(_Pr1 _Pred)
				{
					std::shared_lock lk(lock);
					return std::find_if(container.begin(), container.end(), _Pred);
				}

				void remove(T& obj)
				{
					std::unique_lock lk(lock);
					(void)std::remove(container.begin(), container.end(), obj);
				}

				template<class _Pr1>
				void remove_if(_Pr1 _Pred)
				{
					std::unique_lock lk(lock);
					(void)std::remove_if(container.begin(), container.end(), _Pred);
				}

				template<class _Pr1>
				void sort(_Pr1 _Pred)
				{
					std::unique_lock lk(lock);
					std::sort(container.begin(), container.end(), _Pred);
				}

				std::deque<T>& get_deque_and_lock(std::unique_lock<std::shared_mutex>& lk)
				{
					lk = { lock };
					return container;
				}

				const std::deque<T>& get_deque_and_lock(std::shared_lock<std::shared_mutex>& lk) const
				{
					lk = { lock };
					return container;
				}

				inline void swap(std::deque<T>& swap)
				{
					std::unique_lock lk(lock);
					this->container.swap(swap);
				}

				inline void swap(deque<T>& swap)
				{
					std::unique_lock lk(lock);
					this->container.swap(swap.container);
				}
			};
		}
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif