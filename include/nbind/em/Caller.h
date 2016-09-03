// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

template<typename PolicyList, typename ReturnType, typename... Args>
struct Caller {

	template<typename MethodType, class Bound>
	static typename TypeTransformer<ReturnType, PolicyList>::Binding::WireType callMethod(
		Bound &target,
		MethodType method,
		typename TypeTransformer<Args, PolicyList>::Binding::WireType... args
	) {
		return(TypeTransformer<ReturnType, PolicyList>::Binding::toWireType(
			(target.*method)(ArgFromWire<PolicyList, Args>(args).get(args)...)
		));
	}

	template<typename FunctionType>
	static typename TypeTransformer<ReturnType, PolicyList>::Binding::WireType callFunction(
		FunctionType func,
		typename TypeTransformer<Args, PolicyList>::Binding::WireType... args
	) {
		return(TypeTransformer<ReturnType, PolicyList>::Binding::toWireType(
			(*func)(ArgFromWire<PolicyList, Args>(args).get(args)...)
		));
	}

};

// Specialize Caller for void return type, because toWireType needs a non-void
// argument.

template<typename PolicyList, typename... Args>
struct Caller<PolicyList, void, Args...> {

	template<typename MethodType, class Bound>
	static void callMethod(
		Bound &target,
		MethodType method,
		typename TypeTransformer<Args, PolicyList>::Binding::WireType... args
	) {
		(target.*method)(ArgFromWire<PolicyList, Args>(args).get(args)...);
	}

	template<typename FunctionType>
	static void callFunction(
		FunctionType func,
		typename TypeTransformer<Args, PolicyList>::Binding::WireType... args
	) {
		(*func)(ArgFromWire<PolicyList, Args>(args).get(args)...);
	}

};

} // namespace
