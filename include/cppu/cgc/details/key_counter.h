#pragma once

#include "container_counter.h"

namespace cppu
{
	namespace cgc
	{
		namespace details
		{
			template<class K>
			struct key_counter : public container_counter
			{
				template<typename> friend class strong_ptr;
			protected:
				K key;

				//virtual void add_as_garbage(void* ptr) { container->add_as_garbage(ptr, this); }

			public:
				key_counter(icontainer* container, const K& key, destructor&& destructor)
					: container_counter(container, std::move(destructor))
					, key(key)
				{ }

				inline const K& get_key() const
				{
					return key;
				}
			};
		}
	}
}