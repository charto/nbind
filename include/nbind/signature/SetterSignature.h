// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file is very similar to FunctionSignature.h and MethodSignature.h
// so modify them together.

#pragma once

#include "BaseSignature.h"

namespace nbind {

// Getters and setters come in pairs with a single associated metadata value.
// We need to store separate getter and setter ID numbers as metadata
// so they're packed as 16-bit values into a single 32-bit int.
static constexpr unsigned int accessorSetterShift = 16;

// Wrapper for all C++ getters and setters with matching class and data types.

template <typename PtrType, class Bound, typename ReturnType, typename... Args>
class SetterSignature : public TemplatedBaseSignature<SetterSignature<PtrType, Bound, ReturnType, Args...>, ReturnType, Args...> {

public:

	typedef PtrType MethodType;

	typedef TemplatedBaseSignature<SetterSignature, ReturnType, Args...> Parent;

	static constexpr auto typeExpr = BaseSignature::Type::setter;

#if defined(BUILDING_NODE_EXTENSION)

	template <typename V8Args, typename NanArgs>
	static void callInner(V8Args &args, NanArgs &nanArgs, Bound *target) {
		Parent::CallWrapper::callMethod(
			*target,
			// The static cast silences a compiler warning in Visual Studio.
			Parent::getMethod(static_cast<unsigned int>(nanArgs.Data()->IntegerValue()) >> accessorSetterShift).func,
			args
		);
	}

	static void call(v8::Local<v8::String> property, v8::Local<v8::Value> value, const Nan::PropertyCallbackInfo<void> &args) {
		auto *valuePtr = &value;

		Parent::template callInnerSafely<Bound>(valuePtr, args);
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
