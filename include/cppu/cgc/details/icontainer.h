#pragma once

#include <limits>

typedef unsigned int uint;

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
				virtual void add_as_garbage(void* ptr, const base_counter* c) = 0;
				virtual uint clean_garbage(uint max = 4294967295u) = 0;
			};
		}
	}
}