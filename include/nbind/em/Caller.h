// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

template<typename ReturnType, typename... Args>
struct Caller {

	template<typename MethodType, class Bound>
	static typename BindingType<ReturnType>::WireType callMethod(
		Bound &target,
		MethodType method,
		typename BindingType<Args>::WireType... args
	) {
		return(BindingType<ReturnType>::toWireType(
			(target.*method)(ArgFromWire<Args>(args).get(args)...)
		));
	}

	template<typename FunctionType>
	static typename BindingType<ReturnType>::WireType callFunction(
		FunctionType func,
		typename BindingType<Args>::WireType... args
	) {
		return(BindingType<ReturnType>::toWireType(
			(*func)(ArgFromWire<Args>(args).get(args)...)
		));
	}

};

// Specialize Caller for void return type, because toWireType needs a non-void
// argument.

template<typename... Args>
struct Caller<void, Args...> {

	template<typename MethodType, class Bound>
	static void callMethod(
		Bound &target,
		MethodType method,
		typename BindingType<Args>::WireType... args
	) {
		(target.*method)(ArgFromWire<Args>(args).get(args)...);
	}

	template<typename FunctionType>
	static void callFunction(
		FunctionType func,
		typename BindingType<Args>::WireType... args
	) {
		(*func)(ArgFromWire<Args>(args).get(args)...);
	}

};

} // namespace
