#pragma once

#include <atomic>
#include "icontainer.h"
#include "destructors.h"

namespace cppu
{
	namespace cgc
	{
		template<class T>
		class strong_ptr;

		namespace details
		{
			typedef unsigned short RefCount;

			struct base_counter
			{
				template<typename> friend class ::cppu::cgc::strong_ptr;
			protected:

				// by default
				virtual bool add_as_garbage(void* ptr) const { return false; }

			public:
				std::atomic<RefCount> strongReferences;
				std::atomic<RefCount> weakReferences;
				destructor destruct;

				base_counter(destructor&& destructor)
					: strongReferences(1)
					, weakReferences(0)
					, destruct(std::move(destructor))
				{ }
			};
		}
	}
}