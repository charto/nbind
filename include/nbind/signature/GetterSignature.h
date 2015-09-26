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
static constexpr unsigned int accessorGetterMask = 0xffff;

// Wrapper for all C++ getters and setters with matching class and data types.

template <typename PtrType, class Bound, typename ReturnType, typename... Args>
class GetterSignature : public TemplatedBaseSignature<GetterSignature<PtrType, Bound, ReturnType, Args...>, ReturnType, Args...> {

public:

	typedef PtrType MethodType;

	typedef TemplatedBaseSignature<GetterSignature, ReturnType, Args...> Parent;

	static constexpr auto typeExpr = BaseSignature::Type::getter;

#if defined(BUILDING_NODE_EXTENSION)

	template <typename V8Args, typename NanArgs>
	static void callInner(V8Args &args, NanArgs &nanArgs, Bound *target) {
		nanArgs.GetReturnValue().Set(Parent::CallWrapper::callMethod(
			*target,
			Parent::getMethod(nanArgs.Data()->IntegerValue() & accessorGetterMask).func,
			args
		));
	}

	static void call(v8::Local<v8::String> property, const Nan::PropertyCallbackInfo<v8::Value> &args) {
		// Note: this may do useless arity checks...
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
