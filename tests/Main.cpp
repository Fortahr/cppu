#include <array>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <functional>

#define CPPU_FUNCTION_ENABLE_JUMP_RESOLVE
#include <cppu/function.h>
#include <algorithm>

constexpr size_t shift1 = std::numeric_limits<size_t>::digits - 1;
constexpr size_t shift2 = std::numeric_limits<size_t>::digits - 2;

__declspec(noinline) void FuncPtrTest(int a, size_t b)
{
	std::cout << "YEAH " << a << " " << b << "\n";
}

struct Value1
{
	union target_t
	{
		intptr_t _data;
		intptr_t _address;
		Value1* _ptr;
	} _target;

	union func_t
	{
		void(Value1::* _member)(size_t a, size_t b, size_t& out);
		void(* _static)(size_t a, size_t b, size_t& out);
		intptr_t _address;
	} _func;

	__declspec(noinline) void FuncPtrTest(int a, size_t b)
	{
		std::cout << "YEAP " << a << " " << b << "\n";
	}

	__declspec(noinline) void Message1(size_t a, size_t b, size_t& out)
	{
		out = ((intptr_t)this) + a + b;
	}

	__declspec(noinline) static void MessageStatic(size_t a, size_t b, size_t& out)
	{
		out = a + b;
	}
};

__forceinline void FuncPtrBaseStaticCall(Value1& values, size_t& out)
{
	(*values._func._static)(1, 2, out);
}

__forceinline void FuncPtrBaseMemberCall(Value1& values, size_t& out)
{
	(values._target._ptr->*values._func._member)(1, 2, out);
}

__forceinline void FuncPtrBaseMemberCall_NoDeref(Value1& values, size_t& out)
{
	(values.*values._func._member)(1, 2, out);
}

__forceinline void FuncPtrTest1(Value1& values, size_t& out)
{
	Value1 copy(values);
	if (copy._func._member)
	{

		const auto bits = copy._func._address;
		copy._func._address &= ~((uintptr_t(1) << shift1) | (uintptr_t(1) << shift2));

		if (bits & (uintptr_t(1) << shift2))
		{
			(*copy._func._static)(1, 2, out);
		}
		else
		{
			const intptr_t mask = bits >> shift1;
			copy._target._address = (mask & copy._target._address) | (~mask & (intptr_t)&values._target._address);
			//copy.a = copy.a ^ ((copy.a ^ (intptr_t)&values->a) & mask);
			(copy._target._ptr->*copy._func._member)(1, 2, out);
		}
	}
}

__forceinline void FuncPtrTest2(Value1& values, size_t& out)
{
	Value1 copy(values);
	if (copy._func._member)
	{
		const auto bits = copy._func._address;
		copy._func._address &= ~((uintptr_t(1) << shift1) | (uintptr_t(1) << shift2));

		if (bits & (uintptr_t(1) << shift2))
		{
			(*copy._func._static)(1, 2, out);
		}
		else
		{
			const intptr_t mask = bits >> shift1;
			copy._target._address = mask ? copy._target._address : (intptr_t)&values._target._address;
			(copy._target._ptr->*copy._func._member)(1, 2, out);
		}
	}
}

__forceinline void FuncPtrTestSwitch(Value1& values, size_t& out)
{
	Value1::func_t func = values._func;
	if (func._member)
	{
		const auto bits = (uintptr_t)func._address >> shift2;
		func._address &= ~((uintptr_t(1) << shift1) | (uintptr_t(1) << shift2));

		Value1* target = &values;
		switch (bits)
		{
		case 0:
		case 1:
			target = target->_target._ptr;
			[[fallthrough]];
		case 2:
			(target->*func._member)(1, 2, out);
			break;
		default:
			(*func._static)(1, 2, out);
			break;
		}
	}
}

Value1 value_base_m = { 1, &Value1::Message1 };
Value1 value_base_s = { 1, &Value1::Message1 };

Value1 values[] = {
	{ 1, &Value1::Message1 },
	{ 1, &Value1::Message1 },
	{ 1, &Value1::Message1 }
};

#define RERUNS 9
#define REPEAT 10'000'000

#define TEST(NAME, FUNC, ...) \
	[&]() __declspec(noinline) { \
		double totalTime = 0; \
		std::array<double, RERUNS> runTimes; \
		for (size_t c = 0; c < RERUNS; ++c) \
		{ \
			start = std::chrono::high_resolution_clock::now(); \
			for (size_t i = 0; i < REPEAT; ++i) \
			{ \
				FUNC(__VA_ARGS__); \
			} \
			end = std::chrono::high_resolution_clock::now(); \
			auto time = end - start; \
			totalTime += runTimes[c] = std::chrono::duration<double, std::nano>(time).count(); \
		} \
		std::sort(runTimes.begin(), runTimes.end());\
		constexpr double toTimePerCall = 1.0 / REPEAT; \
		std::cout << std::fixed << std::setprecision(6); \
		std::cout << std::left << std::setw(12) << NAME << " |" << std::right; \
		std::cout << std::setw(14) << (toTimePerCall * runTimes[0]) << " |"; \
		std::cout << std::setw(14) << (toTimePerCall * runTimes[RERUNS / 4]) << " |"; \
		std::cout << std::setw(14) << (toTimePerCall * runTimes[RERUNS / 2]) << " |"; \
		std::cout << std::setw(14) << (toTimePerCall * runTimes[RERUNS - 1 - (RERUNS / 4)]) << " |"; \
		std::cout << std::setw(14) << (toTimePerCall * runTimes[RERUNS - 1]) << " |"; \
		std::cout << std::setw(14) << (toTimePerCall * totalTime / RERUNS) << " |\n"; \
	}();

#define TEST_FOR(NAME, OBJECTS, FUNC, ...) \
	for (auto& object : OBJECTS) \
	{ \
		TEST(NAME, FUNC, object, __VA_ARGS__) \
	} \

#define TEST_HEADER(TEST_NAME, ...) \
	std::cout << std::left << std::setw(12) << TEST_NAME << " |" << std::right; \
	for (auto& name : { __VA_ARGS__ }) \
		std::cout << std::setw(14) << name << " |"; \
	std::cout << '\n';

#define TEST_EMPTY_LINE() \
	std::cout << ' ' << std::setfill('-') << std::right << std::setw(13) << " |"; \
	for (size_t i = 0; i < 6; ++i) \
		std::cout << ' ' << std::setw(15) << " |"; \
	std::cout << '\n' << std::setfill(' ');

struct Functions : Value1
{
	std::function<void(size_t a, size_t b, size_t& out)> stdFunc0;
	cppu::function<void(size_t a, size_t b, size_t& out)> cppuFunc0;

	std::function<void(size_t a, size_t b, size_t& out)> stdFunc1;
	cppu::function<void(size_t a, size_t b, size_t& out)> cppuFunc1;

	std::function<void(size_t a, size_t b, size_t& out)> stdFunc2;
	cppu::function<void(size_t a, size_t b, size_t& out)> cppuFunc2;

	Functions()
	{
		stdFunc0 = &Value1::MessageStatic;
		cppuFunc0 = &Value1::MessageStatic;

		stdFunc1 = std::bind(&Value1::Message1, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		cppuFunc1 = { &Value1::Message1, this };

		auto func = [this](size_t a, size_t b, size_t& out) { out = a + b; };
		stdFunc2 = func;
		cppuFunc2 = func;
	}
};

struct Dummy {};

struct B : public Dummy
{
	virtual void hello() = 0;
};

template<typename _T>
struct D : B
{
	alignas(alignof(_T)) _T value;
};

int main()
{
	std::chrono::high_resolution_clock::time_point start, end;

	constexpr size_t shift1 = std::numeric_limits<size_t>::digits - 1;
	constexpr size_t shift2 = std::numeric_limits<size_t>::digits - 2;
	constexpr size_t mask = (uintptr_t(1) << shift1) | (uintptr_t(1) << shift2);
	constexpr size_t retry = 8;
	constexpr size_t times = 10'000'000;

	Value1 t;
	std::shared_ptr<Value1> p;

	constexpr size_t offset = offsetof(D<__m128>, value);

	value_base_s._func._static = &Value1::MessageStatic;

	values[0]._func._static = &Value1::MessageStatic;
	values[0]._func._address |= uintptr_t(1) << shift2;
	values[1]._func._address |= uintptr_t(1) << shift1;

	Functions* functionTest = new Functions();

	size_t outcome = 0;
	functionTest->stdFunc2(1, 2, outcome);
	std::cout << "STD output: " << outcome << '\n';

	outcome = 0;
	functionTest->cppuFunc2(1, 2, outcome);
	std::cout << "CPPU output: " << outcome << '\n';

	TEST_HEADER("Test", "Min", "1st Quartile", "Median", "3rd Quartile", "Max", "Average");
	TEST_EMPTY_LINE();

	TEST("Base static", FuncPtrBaseStaticCall, value_base_s, outcome);
	TEST("Base member", FuncPtrBaseMemberCall, value_base_m, outcome);
	//TEST("Base member", FuncPtrBaseMemberCall_NoDeref, value_base_m, outcome);
	TEST_EMPTY_LINE();

	TEST("STD static", functionTest->stdFunc0, 1, 0, outcome);
	TEST("CPPU static", functionTest->cppuFunc0, 1, 0, outcome);
	TEST_EMPTY_LINE();

	TEST("STD member", functionTest->stdFunc1, 1, 0, outcome);
	TEST("CPPU member", functionTest->cppuFunc1, 1, 0, outcome);
	TEST_EMPTY_LINE();

	TEST("STD lambda", functionTest->stdFunc2, 1, 0, outcome);
	TEST("CPPU lambda", functionTest->cppuFunc2, 1, 0, outcome);
	TEST_EMPTY_LINE();

	TEST_FOR("Test 1", values, FuncPtrTest1, outcome);
	TEST_EMPTY_LINE();

	TEST_FOR("Test 2", values, FuncPtrTest2, outcome);
	TEST_EMPTY_LINE();

	values[0]._func._address = (values[0]._func._address & ~mask) | (uintptr_t(3) << shift2);
	values[1]._func._address = (values[1]._func._address & ~mask) | (uintptr_t(0) << shift2);
	values[2]._func._address = (values[2]._func._address & ~mask) | (uintptr_t(1) << shift2);
	TEST_FOR("Switch", values, FuncPtrTestSwitch, outcome);/**/

	return 0;
}