// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

template<typename PolicyList, typename ReturnType, typename... Args>
struct Caller {

	typedef typename TypeTransformer<ReturnType, PolicyList>::Binding ReturnBindingType;

	template<typename MethodType, class Bound>
	static typename ReturnBindingType::WireType callMethod(
		Bound &target,
		MethodType method,
		typename TypeTransformer<Args, PolicyList>::WireType... args
	) {
		return(ReturnBindingType::toWireType(
			(target.*method)(ArgFromWire<PolicyList, Args>(args).get(args)...)
		));
	}

	template<typename FunctionType>
	static typename ReturnBindingType::WireType callFunction(
		FunctionType func,
		typename TypeTransformer<Args, PolicyList>::WireType... args
	) {
		return(ReturnBindingType::toWireType(
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
		typename TypeTransformer<Args, PolicyList>::WireType... args
	) {
		(target.*method)(ArgFromWire<PolicyList, Args>(args).get(args)...);
	}

	template<typename FunctionType>
	static void callFunction(
		FunctionType func,
		typename TypeTransformer<Args, PolicyList>::WireType... args
	) {
		(*func)(ArgFromWire<PolicyList, Args>(args).get(args)...);
	}

};

} // namespace
