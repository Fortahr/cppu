#pragma once

#include "../ncm/mutex.h"
#include "array.h"

namespace cppu
{
	namespace cgc
	{
		// Don't destruct this container when any thread is still accessing it, e.g: emplace()
		template<class T, class S = SIZE_32, bool auto_clean = true>
		class m_array
		{
		private:
			std::vector<cgc::array<T, S, auto_clean>*> arrays;
			ncm::mutex lock;

		public:
			m_array()
			{
				cgc::array<T, S, auto_clean>* arr = new cgc::array<T, S, auto_clean>();
				arrays.push_back(arr);
			}

			~m_array()
			{
				lock.lock();

				for (uint i = 0; i < arrays.size(); ++i)
				{
					cgc::array<T, S, auto_clean>* arr = arrays[i];
					delete arr;
				}

				lock.unlock();
			}

			template<class... _Args>
			strong_ptr<T> emplace(_Args&&... arguments)
			{
				cgc::array<T, S, auto_clean>* container = nullptr;
				uint i = 0;

			RETRY_EMPLACE:
				std::size_t size = arrays.size();
				for (; i < size; ++i)
				{
					uint slot = arrays[i]->reserve_spot();
					if (slot != cgc::array<T, S, auto_clean>::npos)
						return arrays[i]->emplace_at(slot, std::forward<_Args>(arguments)...);
				}

				lock.lock();

				// another thread might've created a new array already, retry
				if (size < arrays.size())
				{
					lock.unlock();
					goto RETRY_EMPLACE;
				}

				// no suitable spot found in current arrays, build a new one
				container = new cgc::array<T, S, auto_clean>();
				arrays.push_back(container);

				lock.unlock();

				// Return object
				return container->emplace(std::forward<_Args>(arguments)...);
			}

			inline std::unique_lock<std::mutex> get_lock()
			{
				return std::unique_lock<std::mutex>(lock);
			}

			inline std::vector<cgc::array<T, S, auto_clean>*>& get_arrays()
			{
				return arrays;
			}

			virtual uint clean_garbage(uint max = std::numeric_limits<uint>::max())
			{
				uint newMax = max;
				for (std::size_t i = 0; i < arrays.size(); ++i)
					newMax -= arrays[i]->clean_garbage(newMax);

				return max - newMax;
			}

			inline std::size_t garbage_size()
			{
				uint size = 0;
				for (std::size_t i = 0; i < arrays.size(); ++i)
					size += arrays[i]->garbage_size();

				return size;
			}

			inline bool garbage_empty()
			{
				for (std::size_t i = 0; i < arrays.size(); ++i)
				{
					if (!arrays[i]->garbage_empty())
						return false;
				}

				return true;
			}
		};
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif