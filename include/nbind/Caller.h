// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

class Bindings;

// Caller handles the template magic to compose a method call from a class and
// parts of a method signature extracted from it.

template<typename...> struct TypeList {};

template<typename ArgList> struct Checker;

template<typename... Args>
struct Checker<TypeList<Args...>> {

	template<typename... DummyArgs> static inline void pass(DummyArgs&&...) {}

	static inline bool booleanAndTo(bool valid, bool &validFlag) {
		validFlag &= valid;
		return(valid);
	}

	template <typename NanArgs>
	static bool typesAreValid(NanArgs &args) {
		bool validFlag = true;
		(void)args;	// Silence compile warning about unused parameter.

		pass(booleanAndTo(Args::check(args), validFlag)...);

		return(validFlag);
	}

};

template<typename ReturnType, typename ArgList> struct Caller;

template<typename ReturnType, typename... Args>
struct Caller<ReturnType, TypeList<Args...>> {

	template <class Bound, typename Method, typename NanArgs>
	static ReturnType call(Bound &target, Method method, NanArgs &args) noexcept(false) {
		(void)args;	// Silence compile warning about unused parameter.
		// Note that Args().get may throw.
		return((target.*method)(Args(args).get(args)...));
	}

	template <typename Function, typename NanArgs>
	static ReturnType call(Function func, NanArgs &args) noexcept(false) {
		(void)args;	// Silence compile warning about unused parameter.
		// Note that Args().get may throw.
		return((*func)(Args(args).get(args)...));
	}

};

// Specialize Caller for void return type, because toWireType needs a non-void
// argument.

template<typename... Args>
struct Caller<void,TypeList<Args...>> {

	template <class Bound, typename Method, typename NanArgs>
	static std::nullptr_t call(Bound &target, Method method, NanArgs &args) noexcept(false) {
		(void)args;	// Silence compile warning about unused parameter.
		// Note that Args().get may throw.
		(target.*method)(Args(args).get(args)...);
		return(nullptr);
	}

	template <typename Function, typename NanArgs>
	static std::nullptr_t call(Function func, NanArgs &args) noexcept(false) {
		(void)args;	// Silence compile warning about unused parameter.
		// Note that Args().get may throw.
		(*func)(Args(args).get(args)...);
		return(nullptr);
	}

};

} // namespace
