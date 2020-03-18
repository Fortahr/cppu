#pragma once
#include <mutex>
#include <list>

namespace cppu
{
	namespace stor
	{
		namespace lock
		{
			template<typename T>
			class list
			{
			private:
				std::list<T> ls;
				std::mutex lock;

			public:
				list()
				{}

				list(list& other)
					: ls(other.ls)
				{}

				list& operator=(const list& other)
				{
					ls = other.ls;
					return *this;
				}

				T& operator[](int i)
				{
					return ls[i];
				}

				inline typename std::list<T>::iterator begin()
				{
					return ls.begin();
				}

				inline typename std::list<T>::iterator end()
				{
					return ls.end();
				}

				inline bool empty()
				{
					return ls.empty();
				}

				inline size_t size()
				{
					return ls.size();
				}

				inline void clear()
				{
					std::unique_lock<std::mutex> lk(lock);
					ls.clear();
				}

				inline typename std::list<T>::iterator erase(const typename std::list<T>::const_iterator& it)
				{
					std::unique_lock<std::mutex> lk(lock);
					return ls.erase(it);
				}

				inline typename std::list<T>::iterator erase(typename std::list<T>::iterator& obj)
				{
					std::unique_lock<std::mutex> lk(lock);
					return ls.erase(obj);
				}

				void insert(const typename std::list<T>::const_iterator& insert_from, const typename std::list<T>::const_iterator& from, const typename std::list<T>::const_iterator& to)
				{
					std::unique_lock<std::mutex> lk(lock);
					ls.insert(insert_from, from, to);
				}

				template<class... _Valty>
				void emplace_back(_Valty&&... _Val)
				{
					std::unique_lock<std::mutex> lk(lock);
					ls.emplace_back(std::forward<_Valty>(_Val)...);
				}

				template<class... _Valty>
				void emplace_front(_Valty&&... _Val)
				{
					std::unique_lock<std::mutex> lk(lock);
					ls.emplace_front(std::forward<_Valty>(_Val)...);
				}

				void push_back(const T& obj)
				{
					std::unique_lock<std::mutex> lk(lock);
					ls.push_back(obj);
				}

				void push_front(const T& obj)
				{
					std::unique_lock<std::mutex> lk(lock);
					ls.push_front(obj);
				}

				void pop_back()
				{
					std::unique_lock<std::mutex> lk(lock);
					ls.pop_back();
				}

				void pop_front()
				{
					std::unique_lock<std::mutex> lk(lock);
					ls.pop_front();
				}

				bool pop_back(T& obj)
				{
					std::unique_lock<std::mutex> lk(lock);
					if (!ls.empty())
					{
						obj = ls.back();
						ls.pop_back();

						return true;
					}
					else
						return false;
				}

				bool pop_front(T& obj)
				{
					std::unique_lock<std::mutex> lk(lock);
					if (!ls.empty())
					{
						obj = ls.front();
						ls.pop_front();

						return true;
					}
					else
						return false;
				}

				inline T& front()
				{
					//std::unique_lock<std::mutex> lk(lock);
					return ls.front();
				}

				inline T& back()
				{
					//std::unique_lock<std::mutex> lk(lock);
					return ls.back();
				}

				typename std::list<T>::iterator find(T& obj)
				{
					std::unique_lock<std::mutex> lk(lock);
					return ls.find(obj);
				}

				template<class _Pr1>
				typename std::list<T>::iterator find_if(_Pr1 _Pred)
				{
					std::unique_lock<std::mutex> lk(lock);
					return ls.find_if(_Pred);
				}

				void remove(T& obj)
				{
					std::unique_lock<std::mutex> lk(lock);
					ls.remove(obj);
				}

				template<class _Pr1>
				void remove_if(_Pr1 _Pred)
				{
					std::unique_lock<std::mutex> lk(lock);
					ls.remove_if(_Pred);
				}

				template<class _Pr1>
				void sort(_Pr1 _Pred)
				{
					std::unique_lock<std::mutex> lk(lock);
					ls.sort(_Pred);
				}

				std::list<T>& get_list_and_lock(std::unique_lock<std::mutex>& lk)
				{
					lk = std::unique_lock<std::mutex>(lock);
					return ls;
				}
			};
		}
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif