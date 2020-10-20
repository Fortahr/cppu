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
				
				destructor(destructor&& move) noexcept
					: dtor(move.dtor)
				{
				}

				void operator() (const void* ptr) const
				{
					dtor(ptr);
				}

				template<typename T>
				static destructor create_destructor()
				{
					static auto l = [](const void* x) { static_cast<const T*>(x)->~T(); };
					return destructor(l);
				}
			};
		}
	}
}