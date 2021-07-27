#pragma once

#include "../constructor.h"

namespace cppu
{
	namespace cgc
	{
		namespace details
		{
			struct base_counter;

			template<class V>
			struct counter_value_pair
			{
				friend class constructor;
				template<class K, class V, CLEAN_PROC> friend class cppu::cgc::ref::unordered_map;
			private:
				byte value[sizeof(V)];
				base_counter* counter;

				template<class... _Args>
				counter_value_pair(base_counter* counter, _Args&&... arguments)
					: counter(counter)
				{
					constructor::construct_object<V>(&value, std::forward<_Args>(arguments)...);
				}

				inline base_counter* get_counter() const
				{
					return counter;
				}

				inline V& get_value() const
				{
					return reinterpret_cast<V&>(const_cast<byte&>(value[0]));
				}

			public:
				counter_value_pair()
					: counter(nullptr)
				{ }

				~counter_value_pair()
				{
					// type is unknown by default
					get_value().~V();
				}

				inline V* ptr() const
				{
					return reinterpret_cast<V*>((void*)&value);
				}

				inline V& operator*() const
				{
					return get_value();
				}

				inline V* operator->() const
				{
					return ptr();
				}

				operator strong_ptr<V>() const
				{
					return constructor::construct_pointer(ptr(), counter);
				}

				template <typename = typename std::enable_if<!std::is_const_v<V>>::type>
				operator strong_ptr<const V>() const
				{
					return constructor::construct_pointer<const V>(ptr(), counter);
				}
			};
		}
	}
}