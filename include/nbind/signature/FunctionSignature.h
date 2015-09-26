// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file is very similar to MethodSignature.h and AccessorSignature.h
// so modify them together.

#pragma once

#include "BaseSignature.h"

namespace nbind {

// Wrapper for all C++ functions with matching argument and return types.

template <typename PtrType, class Bound, typename ReturnType, typename... Args>
class FunctionSignature : public TemplatedBaseSignature<FunctionSignature<PtrType, Bound, ReturnType, Args...>, ReturnType, Args...> {

public:

	typedef PtrType MethodType;

	typedef TemplatedBaseSignature<FunctionSignature, ReturnType, Args...> Parent;

	static constexpr auto typeExpr = BaseSignature::Type::function;

	static inline funcPtr getDirect(MethodType func) {
		return(reinterpret_cast<funcPtr>(func));
	}

#if defined(BUILDING_NODE_EXTENSION)

	template <typename V8Args, typename NanArgs>
	static void callInner(V8Args &args, NanArgs &nanArgs, void *) {
		nanArgs.GetReturnValue().Set(Parent::CallWrapper::callFunction(
			Parent::getMethod(nanArgs.Data()->IntegerValue() & signatureMemberMask).func,
			args
		));
	}

	static void call(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		Parent::template callInnerSafely<void>(args, args);
	}

#elif defined(EMSCRIPTEN)

	static typename BindingType<ReturnType>::WireType call(
		uint32_t num,
		typename BindingType<Args>::WireType... args
	) {
		auto func = Parent::getMethod(num).func;

		return(Caller<ReturnType, Args...>::callFunction(func, args...));
	}

#endif // BUILDING_NODE_EXTENSION, EMSCRIPTEN

};

} // namespace
