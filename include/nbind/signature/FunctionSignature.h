// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file is very similar to MethodSignature.h and AccessorSignature.h
// so modify them together.

#pragma once

#include "BaseSignature.h"

namespace nbind {

// Wrapper for all C++ functions with matching argument and return types.

template <typename PtrType, class Bound, typename PolicyList, typename ReturnType, typename... Args>
class FunctionSignature : public TemplatedBaseSignature<FunctionSignature<PtrType, Bound, PolicyList, ReturnType, Args...>, PolicyList, ReturnType, Args...> {

public:

	typedef PtrType MethodType;

	typedef TemplatedBaseSignature<FunctionSignature, PolicyList, ReturnType, Args...> Parent;

	static constexpr auto typeExpr = BaseSignature :: SignatureType :: func;

	static inline funcPtr getDirect(MethodType func) {
		return(reinterpret_cast<funcPtr>(func));
	}

#if defined(BUILDING_NODE_EXTENSION)

	template <typename V8Args, typename NanArgs>
	static void callInner(const typename Parent::MethodInfo &method, V8Args &args, NanArgs &nanArgs, void *) {
		nanArgs.GetReturnValue().Set(Parent::CallWrapper::callFunction(
			method.func,
			args
		));
	}

	static void call(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		Parent::template callInnerSafely<void>(
			args,
			args,
			SignatureParam::get(args)->methodNum
		);
	}

#elif defined(__EMSCRIPTEN__)

	static typename TypeTransformer<ReturnType, PolicyList>::Binding::WireType call(
		uint32_t num,
		typename TypeTransformer<Args, PolicyList>::Binding::WireType... args
	) {
		auto func = Parent::getMethod(num).func;

		return(Caller<PolicyList, ReturnType, Args...>::callFunction(func, args...));
	}

#endif // BUILDING_NODE_EXTENSION, __EMSCRIPTEN__

};

} // namespace
