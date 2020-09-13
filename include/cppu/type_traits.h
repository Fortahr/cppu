#pragma once

#include <type_traits>
#include <memory>

namespace cppu
{
	namespace cgc
	{
		template<class T> class strong_ptr;
		template<class T> class weak_ptr;
	}

	template<class T> struct is_smart_ptr : std::false_type {};
	
	template<class T> struct is_smart_ptr<std::unique_ptr<T>> : std::true_type {};
	template<class T> struct is_smart_ptr<std::shared_ptr<T>> : std::true_type {};
	template<class T> struct is_smart_ptr<std::weak_ptr<T>> : std::true_type {};
	
	template<class T> struct is_smart_ptr<::cppu::cgc::strong_ptr<T>> : std::true_type {};
	template<class T> struct is_smart_ptr<::cppu::cgc::weak_ptr<T>> : std::true_type {};

	template<class T> inline constexpr bool is_smart_ptr_v = is_smart_ptr<T>::value;

	template<typename T>
	struct has_const_iterator
	{
	private:
		typedef char                      yes;
		typedef struct { char array[2]; } no;

		template<typename C> static yes test(typename C::const_iterator*);
		template<typename C> static no  test(...);
	public:
		static const bool value = sizeof(test<T>(0)) == sizeof(yes);
		typedef T type;
	};

	template <typename T>
	struct has_begin_and_end
	{
		template<typename C> static char (&test(typename std::enable_if<
			std::is_same<decltype(static_cast<typename C::const_iterator(C::*)() const>(&C::begin)), typename C::const_iterator(C::*)() const>::value
			&& std::is_same<decltype(static_cast<typename C::const_iterator(C::*)() const>(&C::end)), typename C::const_iterator(C::*)() const>::value
			, void>::type*))[1];

		template<typename C> static char (&test(...))[2];

		static bool const value = sizeof(test<T>(0)) == 1;
	};
	
	// is an STL container type
	template<typename T>
	struct is_container : std::integral_constant<bool, has_const_iterator<T>::value && has_begin_and_end<T>::value>
	{ };

	// Is string check
	template<typename T>
	struct is_string : public std::integral_constant<bool, std::is_same<char*, typename std::decay<T>::type>::value || std::is_same<const char*, typename std::decay<T>::type>::value> {};

	template<>
	struct is_string<std::string> : std::true_type {};
}