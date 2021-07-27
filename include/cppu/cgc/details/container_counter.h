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

				virtual void add_as_garbage(void* ptr) const override
				{
					if (!container->add_as_garbage(ptr, this))
						base_counter::add_as_garbage(ptr);
				}

			public:
				container_counter(icontainer* container, destructor&& destructor)
					: base_counter(std::move(destructor))
					, container(container)
				{ }
			};
		}
	}
}