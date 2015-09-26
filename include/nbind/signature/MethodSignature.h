// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file is very similar to FunctionSignature.h and AccessorSignature.h
// so modify them together.

#pragma once

#include "BaseSignature.h"

namespace nbind {

// Wrapper for all C++ methods with matching class, argument and return types.

template <typename PtrType, class Bound, typename ReturnType, typename... Args>
class MethodSignature : public TemplatedBaseSignature<MethodSignature<PtrType, Bound, ReturnType, Args...>, ReturnType, Args...> {

public:

	typedef PtrType MethodType;

	typedef TemplatedBaseSignature<MethodSignature, ReturnType, Args...> Parent;

	static constexpr auto typeExpr = BaseSignature::Type::method;

#if defined(BUILDING_NODE_EXTENSION)

	template <typename V8Args, typename NanArgs>
	static void callInner(V8Args &args, NanArgs &nanArgs, Bound *target) {
		nanArgs.GetReturnValue().Set(Parent::CallWrapper::callMethod(
			*target,
			Parent::getMethod(nanArgs.Data()->IntegerValue() & signatureMemberMask).func,
			args
		));
	}

	static void call(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		Parent::template callInnerSafely<Bound>(args, args);
	}

#elif defined(EMSCRIPTEN)

	static typename BindingType<ReturnType>::WireType call(
		uint32_t num,
		Bound *target,
		typename BindingType<Args>::WireType... args
	) {
		auto method = Parent::getMethod(num).func;

		return(Caller<ReturnType, Args...>::callMethod(*target, method, args...));
	}

#endif // BUILDING_NODE_EXTENSION, EMSCRIPTEN

};

} // namespace
