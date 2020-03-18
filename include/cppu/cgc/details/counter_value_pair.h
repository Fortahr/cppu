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
			private:
				byte value[sizeof(V)];
				base_counter* counter;

			public:
				template<class... _Args>
				counter_value_pair()
					: counter(nullptr)
				{
				}

				template<class... _Args>
				counter_value_pair(base_counter* counter, _Args&&... arguments)
					: counter(counter)
				{
					constructor::construct_object<V>(&value, std::forward<_Args>(arguments)...);
				}

				~counter_value_pair()
				{
					// type is unknown by default
					reinterpret_cast<V&>(value).~V();
				}

				operator strong_ptr<V>()
				{
					return constructor::construct_pointer(&get_value(), counter);
				}

				inline V& get_value()
				{
					return reinterpret_cast<V&>(value);
				}

				inline base_counter* get_counter()
				{
					return counter;
				}
			};
		}
	}
}