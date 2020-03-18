#pragma once

#include <iostream>

namespace cppu
{
	namespace cgc
	{
		namespace details
		{
			struct destructor
			{
				void(*dtor)(const void*);
				
				destructor(void(*dtor)(const void*))
					: dtor(dtor)
				{
				}
				
				void operator() (const void* ptr) const
				{
					dtor(ptr);
				}

				static destructor destruct;
			};
		}
	}
}