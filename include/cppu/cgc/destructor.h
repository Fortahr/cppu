#pragma once

#include <type_traits>
#include "details/types.h"
#include "details/base_counter.h"
#include "details/destructors.h"

namespace cppu
{
	namespace cgc
	{
		template<typename> class strong_ptr;
		template<typename> class weak_ptr;

		template<typename, typename, CLEAN_PROC> class array;
		template<typename, typename, bool> class deque;
		template<typename, typename, bool> class queue;
		template<typename, typename, bool> class map;
		template<typename, typename, CLEAN_PROC> class unordered_map;

		namespace ref
		{
			template<typename, typename, CLEAN_PROC> class array;
			template<typename, typename, bool> class deque;
			template<typename, typename, bool> class queue;
			template<typename, typename, bool> class map;
			template<typename, typename, CLEAN_PROC> class unordered_map;
		}

		namespace details
		{
			template<typename> struct counter_value_pair;
			template<typename, typename> class base_unordered_map;
			struct base_counter;
		}

		class constructor
		{
			// friends
			template<typename, typename, CLEAN_PROC> friend class array;
			template<typename, typename, bool> friend class deque;
			template<typename, typename, bool> friend class queue;
			template<typename, typename, bool> friend class map;
			template<typename, typename, CLEAN_PROC> friend class unordered_map;

			template<typename, typename, CLEAN_PROC> friend class ref::array;
			template<typename, typename, bool> friend class ref::deque;
			template<typename, typename, bool> friend class ref::queue;
			template<typename, typename, bool> friend class ref::map;
			template<typename, typename, CLEAN_PROC> friend class ref::unordered_map;

			template<typename, typename> friend class details::base_unordered_map;
			template<typename> friend struct details::counter_value_pair;
			template<typename T, typename... Args> friend strong_ptr<T> construct_new(Args&&...);
			template<typename T, typename... Args> friend strong_ptr<T> gcnew(Args&& ...);

		private:
			template<class T, class... Args>
			inline static void construct_object(void* pointer, Args&&... arguments)
			{
				new(pointer) T(std::forward<Args>(arguments)...);
			}

			template<class T>
			inline static strong_ptr<T> construct_pointer(T* pointer, details::base_counter* count)
			{
				return strong_ptr<T>(pointer, count);
			}

			template<class T, class... Args>
			inline static strong_ptr<T> construct_object_via_function(Args&&... arguments)
			{
				return T::construct(std::forward<Args>(arguments)...);
			}
		};

		template<class T, class... Args>
		inline strong_ptr<T> construct_new(Args&&... arguments)
		{
			T* object = new T(std::forward<Args>(arguments)...);
			return constructor::construct_pointer(object, new details::base_counter([](const void* x) { static_cast<const T*>(x)->~T(); }));
		}

		template<class T, class... Args>
		inline strong_ptr<T> gcnew(Args&&... arguments)
		{
			T* object = new T(std::forward<Args>(arguments)...);
			return constructor::construct_pointer(object, new details::base_counter([](const void* x) { static_cast<const T*>(x)->~T(); }));
		}
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif