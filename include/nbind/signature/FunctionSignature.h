// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file is very similar to MethodSignature.h and AccessorSignature.h
// so modify them together.

#pragma once

#include "BaseSignature.h"

namespace nbind {

// Wrapper for all C++ functions with matching argument and return types.

template <typename ReturnType, typename... Args>
class FunctionSignature : public TemplatedBaseSignature<FunctionSignature<ReturnType, Args...>, ReturnType, Args...> {

public:

	typedef ReturnType(*MethodType)(Args...);

	typedef TemplatedBaseSignature<FunctionSignature, ReturnType, Args...> Parent;

	static constexpr auto typeExpr = BaseSignature::Type::function;

#ifdef BUILDING_NODE_EXTENSION
	template <typename V8Args, typename NanArgs>
	static bool callInner(V8Args &args, NanArgs &nanArgs, void *) {
		auto result = Parent::CallWrapper::call(
			Parent::getMethod(nanArgs.Data()->IntegerValue() & signatureMemberMask).func,
			args
		);

		if(Status::getError() != nullptr) return(false);

		nanArgs.GetReturnValue().Set(BindingType<ReturnType>::toWireType(std::move(result)));
		return(true);
	}

	static void call(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		Parent::template callInnerSafely<void>(args, args);
	}
#else
	static void call() {}
#endif // BUILDING_NODE_EXTENSION

};

} // namespace
