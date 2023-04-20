#pragma once

#include <limits>

namespace cppu
{
	namespace cgc
	{
		namespace details
		{
			struct base_counter;

			class icontainer
			{
			public:
				virtual bool add_as_garbage(void* ptr, const base_counter* c) = 0;
				virtual size_t clean_garbage(size_t max = 4294967295u) = 0;
			};
		}
	}
}