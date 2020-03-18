#pragma once

#include "base_counter.h"

namespace cppu
{
	namespace cgc
	{
		namespace details
		{
			struct container_counter : public base_counter
			{
				template<typename> friend class strong_ptr;
			protected:
				icontainer* container;

				virtual bool add_as_garbage(void* ptr) const
				{
					container->add_as_garbage(ptr, this);
					return true;
				}

			public:
				container_counter(icontainer* container, void(*dtor)(const void*))
					: base_counter(dtor)
					, container(container)
				{ }
			};
		}
	}
}