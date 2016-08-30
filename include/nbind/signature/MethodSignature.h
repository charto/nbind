// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file is very similar to FunctionSignature.h and AccessorSignature.h
// so modify them together.

#pragma once

#include "BaseSignature.h"

namespace nbind {

// Wrapper for all C++ methods with matching class, argument and return types.

template <typename PtrType, class Bound, typename PolicyList, typename ReturnType, typename... Args>
class MethodSignature : public TemplatedBaseSignature<MethodSignature<PtrType, Bound, PolicyList, ReturnType, Args...>, PolicyList, ReturnType, Args...> {

public:

	typedef PtrType MethodType;

	typedef TemplatedBaseSignature<MethodSignature, PolicyList, ReturnType, Args...> Parent;

	static constexpr auto typeExpr = BaseSignature :: SignatureType :: method;

#if defined(BUILDING_NODE_EXTENSION)

	template <typename V8Args, typename NanArgs>
	static void callInner(const typename Parent::MethodInfo &method, V8Args &args, NanArgs &nanArgs, Bound *target) {
		nanArgs.GetReturnValue().Set(Parent::CallWrapper::callMethod(
			*target,
			method.func,
			args
		));
	}

	static void call(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		Parent::template callInnerSafely<Bound>(
			args,
			args,
			args.Data()->Uint32Value() & signatureMemberMask
		);
	}

#elif defined(EMSCRIPTEN)

	static typename BindingType<ReturnType>::WireType call(
		uint32_t num,
		Bound *target,
		typename BindingType<Args>::WireType... args
	) {
		auto method = Parent::getMethod(num).func;

		return(Caller<PolicyList, ReturnType, Args...>::callMethod(*target, method, args...));
	}

#endif // BUILDING_NODE_EXTENSION, EMSCRIPTEN

};

} // namespace
