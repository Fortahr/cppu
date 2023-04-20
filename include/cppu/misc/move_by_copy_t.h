#pragma once

/// \brief will copy instead of move, e.g.: can be used for std::mutex
namespace cppu
{
	template<typename _T>
	class move_by_copy_t : public _T
	{
	public:
		move_by_copy_t() : _T() { }
		move_by_copy_t(const move_by_copy_t&) : _T() { }

		move_by_copy_t<_T>& operator=(const move_by_copy_t&) { return *new(this) _T(); }
	};
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif