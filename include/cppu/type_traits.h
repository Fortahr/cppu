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

	template<typename T> inline constexpr bool is_container_v = is_container<T>::value;

	// Is string check
	template<typename T>
	struct is_string : public std::integral_constant<bool, std::is_same_v<char*, typename std::decay_t<T>> || std::is_same_v<const char*, typename std::decay_t<T>>> {};

	template<>
	struct is_string<std::string> : std::true_type {};

	template <typename T, typename = void>
	struct is_map : std::false_type {};

	template <typename T>
	struct is_map<T, std::enable_if_t<std::is_same<typename T::value_type, std::pair<const typename T::key_type, typename T::mapped_type>>::value>> : std::true_type {};

	template<typename T> inline constexpr bool is_map_v = is_map<T>::value;

	template<typename T> struct is_vector : public std::false_type {};
	template<typename T, typename A> struct is_vector<std::vector<T, A>> : public std::true_type {};

	template<typename T> inline constexpr bool is_vector_v = is_vector<T>::value;


}