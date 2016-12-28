// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file is very similar to FunctionSignature.h and MethodSignature.h
// so modify them together.

#pragma once

#include "BaseSignature.h"

namespace nbind {

// Wrapper for all C++ getters and setters with matching class and data types.

template <typename PtrType, class Bound, typename PolicyList, typename ReturnType, typename... Args>
class SetterSignature : public TemplatedBaseSignature<SetterSignature<PtrType, Bound, PolicyList, ReturnType, Args...>, PolicyList, ReturnType, Args...> {

public:

	typedef PtrType MethodType;

	typedef TemplatedBaseSignature<SetterSignature, PolicyList, ReturnType, Args...> Parent;

	static constexpr auto typeExpr = BaseSignature :: SignatureType :: setter;

#if defined(BUILDING_NODE_EXTENSION)

	template <typename V8Args, typename NanArgs>
	static void callInner(const typename Parent::MethodInfo &method, V8Args &args, NanArgs &nanArgs, Bound *target) {
		Parent::CallWrapper::callMethod(
			*target,
			method.func,
			args
		);
	}

	static void call(v8::Local<v8::String> property, v8::Local<v8::Value> value, const Nan::PropertyCallbackInfo<void> &args) {
		auto *valuePtr = &value;

		Parent::template callInnerSafely<Bound>(
			valuePtr,
			args,
			SignatureParam::get(args)->setterNum
		);
	}

#elif defined(__EMSCRIPTEN__)

	static typename TypeTransformer<ReturnType, PolicyList>::Binding::WireType call(
		uint32_t num,
		Bound *target,
		typename TypeTransformer<Args, PolicyList>::Binding::WireType... args
	) {
		auto method = Parent::getMethod(num).func;

		return(Caller<PolicyList, ReturnType, Args...>::callMethod(*target, method, args...));
	}

#endif // BUILDING_NODE_EXTENSION, __EMSCRIPTEN__

};

} // namespace
