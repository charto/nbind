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

template <class Bound, typename ReturnType, typename... Args>
class GetterSignature : public TemplatedBaseSignature<GetterSignature<Bound, ReturnType, Args...>, ReturnType, Args...> {

public:

	typedef ReturnType(Bound::*MethodType)(Args...);

	typedef TemplatedBaseSignature<GetterSignature, ReturnType, Args...> Parent;

	static constexpr auto typeExpr = BaseSignature::Type::getter;

#ifdef BUILDING_NODE_EXTENSION
	template <typename V8Args, typename NanArgs>
	static bool callInner(V8Args &args, NanArgs &nanArgs, Bound *target) {
		auto &&result = Caller<
			ReturnType,
			TypeList<>
		>::call(
			*target,
			Parent::getMethod(nanArgs.Data()->IntegerValue() & accessorGetterMask).func,
			args
		);

		nanArgs.GetReturnValue().Set(BindingType<ReturnType>::toWireType(std::move(result)));
		if(Status::getError() != nullptr) return(false);

		return(true);
	}

	static void call(v8::Local<v8::String> property, const Nan::PropertyCallbackInfo<v8::Value> &args) {
		// Note: this may do useless arity checks...
		Parent::template callInnerSafely<Bound>(args, args);
	}
#else
	static void call() {}
#endif // BUILDING_NODE_EXTENSION

};

} // namespace
