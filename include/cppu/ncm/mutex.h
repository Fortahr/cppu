#pragma once

#include <mutex>

/// \brief No copy/move variant of the mutex object
namespace cppu
{
	namespace ncm
	{
		class mutex : public std::mutex
		{
		public:
			mutex() : std::mutex() { }
			mutex(const mutex&) : std::mutex() { }
			//mutex(mutex&&) noexcept : std::mutex() { }

			mutex& operator=(const mutex&) { return *new(this) mutex(); }
			//mutex& operator=(mutex&&) noexcept { return *new(this) mutex(); }
		};
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif