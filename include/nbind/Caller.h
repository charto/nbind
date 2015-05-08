// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

class Bindings;

// Caller handles the template magic to compose a method call from a class and
// parts of a method signature extracted from it.

template<typename...> struct TypeList {};

template<typename ReturnType,typename ArgList> struct Caller;

template<typename ReturnType,typename... Args>
struct Caller<ReturnType,TypeList<Args...>> {

	template <class Bound, typename Method, typename NanArgs>
	static ReturnType call(Bound &target, Method method, NanArgs args) {
		(void)args;	// Silence compile warning about unused parameter.
		return((target.*method)(Args(args).get()...));
	}

	template <typename Function, typename NanArgs>
	static ReturnType call(Function func, NanArgs args) {
		(void)args;	// Silence compile warning about unused parameter.
		return((*func)(Args(args).get()...));
	}

};

// Specialize Caller for void return type, because toWireType needs a non-void
// argument.

template<typename... Args>
struct Caller<void,TypeList<Args...>> {

	template <class Bound, typename Method, typename NanArgs>
	static std::nullptr_t call(Bound &target, Method method, NanArgs args) {
		(void)args;	// Silence compile warning about unused parameter.
		(target.*method)(Args(args).get()...);
		return(nullptr);
	}

	template <typename Function, typename NanArgs>
	static std::nullptr_t call(Function func, NanArgs args) {
		(void)args;	// Silence compile warning about unused parameter.
		(*func)(Args(args).get()...);
		return(nullptr);
	}

};

} // namespace
