#pragma once

#include <type_traits>
#include <memory>

namespace cppu
{
	namespace details
	{
		template<typename _Func, typename _R, typename... _Args>
		struct is_lambda_invocable : std::false_type {};

		template<typename _R2, typename _T2, typename... _Args2, typename _R, typename... _Args>
		struct is_lambda_invocable<_R2(_T2::*)(_Args2...), _R, _Args...> : std::is_invocable_r<_R, _R2(_Args2...), _Args...>
		{ };

		template<typename _R2, typename _T2, typename... _Args2, typename _R, typename... _Args>
		struct is_lambda_invocable<_R2(_T2::*)(_Args2...) const, _R, _Args...> : std::is_invocable_r<_R, _R2(_Args2...), _Args...>
		{ };

		template<typename _R2, typename... _Args2, typename _R, typename... _Args>
		struct is_lambda_invocable<_R2(*)(_Args2...), _R, _Args...> : std::is_invocable_r<_R, _R2(_Args2...), _Args...>
		{ };

		template<typename _R2, typename... _Args2, typename _R, typename... _Args>
		struct is_lambda_invocable<_R2(_Args2...), _R, _Args...> : std::is_invocable_r<_R, _R2(_Args2...), _Args...>
		{ };

		template<typename _Func, typename _R, typename... _Args>
		inline constexpr bool is_lambda_invocable_v = is_lambda_invocable<_Func, _R, _Args...>::value;

		template<typename _T, size_t _MaxSize>
		struct is_embeddable : std::integral_constant<bool,
			std::is_trivially_move_constructible_v<_T>&& std::is_trivially_destructible_v<_T>
			&& sizeof(_T) <= _MaxSize && alignof(_T) <= alignof(void*)>
		{ };

		template<typename _T, size_t _MaxSize>
		inline constexpr bool is_embeddable_v = is_embeddable<_T, _MaxSize>::value;

		struct dummy { };

		template <typename _B = dummy>
		class closure_t;

		template <>
		class __declspec(novtable) closure_t<dummy> : public dummy
		{
		public:
			virtual closure_t* copy() const = 0;
			virtual void destruct() = 0;
		};

		template <typename _B>
		class closure_t : public closure_t<dummy>
		{
		public:
			closure_t() = default;

			closure_t(const _B& copy)
			{
				new (static_cast<dummy*>(this)) _B(copy);
			}

			closure_t(_B&& move)
			{
				new (static_cast<dummy*>(this)) _B(std::move(move));
			}

			virtual closure_t* copy() const
			{
				auto ptr = new closure_t();
				auto* data = (const _B*)static_cast<const dummy*>(this);
				new (static_cast<dummy*>(ptr)) _B(*data);
				return ptr;
			}

			virtual void destruct()
			{
				reinterpret_cast<_B*>(static_cast<dummy*>(this))->~_B();
				delete static_cast<closure_t*>(this);
			}

			void* operator new(size_t size)
			{
#ifdef _WIN32
				return _aligned_offset_malloc(sizeof(closure_t<>) + sizeof(_B), alignof(_B), sizeof(closure_t<>));
#endif
			}

			void operator delete(void* ptr) noexcept
			{
#ifdef _WIN32
				_aligned_free(ptr);
#endif
			}
		};

#ifdef CPPU_FUNCTION_ENABLE_JUMP_RESOLVE

#if defined(__x86_64__) || defined(_M_X64) || defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
#define IS_X86
#endif

#pragma pack(push, 1) // we need that op code and offset right next to each other
		struct jmp_check_t
		{
#ifdef IS_X86
			constexpr static uint8_t rel_jmp = 0xE9;
#endif
			uint8_t _opCode;
			uint32_t _offset;
		};
#pragma pack(pop)

		template <typename _Func>
		inline _Func resolve_jumps(_Func func)
		{
#ifdef IS_X86
			union { _Func src; jmp_check_t* jmp; };
			src = func;

			while (jmp->_opCode == jmp_check_t::rel_jmp)
				jmp = (jmp_check_t*)((intptr_t)jmp + jmp->_offset + sizeof(jmp_check_t));

			return src;
#else
			return func;
#endif
		}
#else
		template <typename _Func>
		inline _Func resolve_jumps(_Func func)
		{
			return func;
		}
#endif
	}

	class bad_function_call {};

	template <typename _R, typename... _Args>
	class function;

	template<typename _R, typename... _Args>
	class function<_R(_Args...)>
	{
	private:
		enum tag_e : uintptr_t
		{
			TARGET_LESS = 0,
			TARGET_EMBEDDED,
			TARGET_UNOWNED,
			TARGET_OWNED,

			SHIFT = std::numeric_limits<uintptr_t>::digits - 2,
			MASK = uintptr_t(0b11) << SHIFT,

			TARGET_UNOWNED_SHIFTED = TARGET_UNOWNED << SHIFT,
			TARGET_OWNED_SHIFTED = TARGET_OWNED << SHIFT,
			TARGET_EMBEDDED_SHIFTED = TARGET_EMBEDDED << SHIFT,
			TARGET_LESS_SHIFTED = TARGET_LESS << SHIFT
		};

		union target_t
		{
			details::dummy* _ptr;
			details::dummy _embed;

			target_t() noexcept = default;
			target_t(const target_t&) noexcept = default;
			target_t(target_t&& move) noexcept : _ptr(move._ptr) { move._ptr = nullptr; };
			target_t(std::nullptr_t) noexcept : target_t() { }

			target_t(details::dummy* ptr) noexcept : _ptr(ptr) { }
			target_t(details::closure_t<>* ptr) noexcept : _ptr(static_cast<details::dummy*>(ptr)) { }

			void operator=(std::nullptr_t) noexcept { _ptr = nullptr; }
			void operator=(const target_t& copy) noexcept { _ptr = copy._ptr; }
			void operator=(target_t&& move) noexcept { _ptr = move._ptr; move._ptr = nullptr; }

			operator bool() const noexcept { return _ptr != nullptr; }
			bool operator==(const target_t& right) const noexcept { return _ptr == right._ptr; }
		} _target;

		union func_t
		{
			_R(details::dummy::* _member)(_Args...);
			_R(*_static)(_Args...);
			uintptr_t _address;

			func_t() noexcept = default;
			func_t(const func_t&) noexcept = default;
			func_t(func_t&& move) noexcept : _member(move._member) { move._member = nullptr; }
			func_t(std::nullptr_t) noexcept : target_t() { }

			func_t(_R(details::dummy::* func)(_Args...)) noexcept : _member(details::resolve_jumps(func)) { }
			func_t(_R(*func)(_Args...)) noexcept : _static(details::resolve_jumps(func)) { }

			void operator=(_R(details::dummy::* func)(_Args...)) noexcept { _member = func; }
			void operator=(_R(*func)(_Args...)) noexcept { _static = func; }

			void operator=(std::nullptr_t) noexcept { _member = nullptr; }
			void operator=(const func_t& copy) noexcept { _member = copy._member; }
			void operator=(func_t&& move) noexcept { _member = move._member; move._member = nullptr; }

			operator bool() const noexcept { return _member != nullptr; }
			bool operator==(const func_t& right) const noexcept { return _member == right._member; }
		} _func;

		inline bool target_owned() const noexcept
		{
			return (_func._address & tag_e::MASK) == tag_e::TARGET_OWNED_SHIFTED;
		}

		inline details::dummy* target_copy() const noexcept
		{
			return target_owned()
				? static_cast<details::dummy*>(static_cast<details::closure_t<>*>(_target._ptr)->copy())
				: _target._ptr;
		}

		inline void target_destruct()
		{
			if (target_owned())
				static_cast<details::closure_t<>*>(_target._ptr)->destruct();
		}

		template<typename _Func>
		inline static _R(details::dummy::* reinterpret_func(_Func func))(_Args...)
		{
			// the object pointer adjustor is pre-applied as we don't support the casting of member functions
			union { _Func src; _R(details::dummy::* dst)(_Args...); } ptr = { func };
			return ptr.dst;
		}

	public:
		function()
			: _target()
			, _func()
		{ }

		function(const function& copy)
			: _target(copy.target_copy())
			, _func(copy._func)
		{ }

		function(function&& move) noexcept
			: _target(std::move(move._target))
			, _func(std::move(move._func))
		{ }

		// static function
		function(_R(*func)(_Args...)) noexcept
			: _target()
			, _func(func)
		{
			_func._address |= tag_e::TARGET_LESS_SHIFTED;
		}

		// member function
		template<typename _B, typename _T, typename = std::enable_if_t<std::is_base_of_v<_B, _T>>>
		function(_R(_B::* func)(_Args...), _T* target) noexcept
			: _target((details::dummy*)static_cast<_B*>(target))
			, _func(reinterpret_func(func))
		{
			_func._address |= tag_e::TARGET_UNOWNED_SHIFTED;
		}

		// member function embedded
		template<typename _B, typename _T, typename = std::enable_if_t<std::is_base_of_v<_B, _T>>>
		function(_R(_B::* func)(_Args...), const _T& target) noexcept
			: _func(reinterpret_func(func))
		{
			if constexpr (details::is_embeddable_v<_T, sizeof(target_t)>)
			{
				new (&_target) _T(target);
				_func._address |= tag_e::TARGET_EMBEDDED_SHIFTED;
			}
			else
			{
				new (&_target) target_t(new details::closure_t<_T>(target));
				_func._address |= tag_e::TARGET_OWNED_SHIFTED;
			}
		}

		// member function embedded
		template<typename _B, typename _T, typename = std::enable_if_t<std::is_base_of_v<_B, _T>>>
		function(_R(_B::* func)(_Args...), _T&& target) noexcept
			: _func(reinterpret_func(func))
		{
			if constexpr (details::is_embeddable_v<_T, sizeof(target_t)>)
			{
				new (&_target) _T(std::move(target));
				_func._address |= tag_e::TARGET_EMBEDDED_SHIFTED;
			}
			else
			{
				new (&_target) target_t(new details::closure_t<_T>(std::move(target)));
				_func._address |= tag_e::TARGET_OWNED_SHIFTED;
			}
		}

		// lambda function
		template<typename _Lambda, typename =
			std::enable_if_t<details::is_lambda_invocable_v<decltype(&std::decay_t<_Lambda>::operator()), _R, _Args...>>>
		function(_Lambda&& lambda) noexcept
			: _func(reinterpret_func(&std::decay_t<_Lambda>::operator()))
		{
			typedef std::decay_t<_Lambda> _DecayedLambda;
			if constexpr (details::is_embeddable_v<_DecayedLambda, sizeof(target_t)>)
			{
				new (&_target) _DecayedLambda(std::move(lambda));
				_func._address |= tag_e::TARGET_EMBEDDED_SHIFTED;
			}
			else
			{
				new (&_target) target_t(new details::closure_t<_DecayedLambda>(std::move(lambda)));
				_func._address |= tag_e::TARGET_OWNED_SHIFTED;
			}
		}

		~function()
		{
			target_destruct();
		}

		function& operator=(const function& copy)
		{
			target_destruct();

			_target = copy.target_copy();
			_func = copy._func;

			return *this;
		}

		function& operator=(function&& move) noexcept
		{
			target_destruct();

			_target = std::move(move._target);
			_func = std::move(move._func);

			return *this;
		}

		function& operator=(std::nullptr_t)
		{
			target_destruct();

			_target = nullptr;
			_func = nullptr;

			return *this;
		}

		inline operator bool() const noexcept
		{
			return _func;
		}

		inline bool operator==(const function& right) const noexcept
		{
			return _target == right._target && _func == right._func;
		}

		__forceinline _R operator()(_Args... args) const
		{
			target_t target = _target;
			func_t func = _func;

			const uint_fast8_t tag = func._address >> tag_e::SHIFT;
			func._address &= ~tag_e::MASK;

#ifdef _MSC_VER
			__assume(tag < 4);
#endif
			if (tag)
			{
				details::dummy* obj = !(tag & 0b10) ? const_cast<details::dummy*>(&_target._embed) : target._ptr;
				return (obj->*func._member)(std::forward<_Args>(args)...);
			}
			else if (func)
				return (*func._static)(std::forward<_Args>(args)...);

			throw bad_function_call();
		}
	};
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif