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

template <class Bound, typename ReturnType, typename... Args>
class SetterSignature : public TemplatedBaseSignature<SetterSignature<Bound, ReturnType, Args...>, ReturnType, Args...> {

public:

	typedef ReturnType(Bound::*MethodType)(Args...);

	typedef TemplatedBaseSignature<SetterSignature, ReturnType, Args...> Parent;

#ifdef BUILDING_NODE_EXTENSION
	static void call(v8::Local<v8::String> property, v8::Local<v8::Value> value, const Nan::PropertyCallbackInfo<void> &args) {

		v8::Local<v8::Object> targetWrapped = args.This();
		Bound &target = node::ObjectWrap::Unwrap<BindWrapper<Bound>>(targetWrapped)->getBound();

		Status::clearError();

		auto *valuePtr = &value;

		if(!SetterSignature::typesAreValid(valuePtr)) {
			Nan::ThrowTypeError("Type mismatch");
		} else {
			try {
				Parent::CallWrapper::call(target, Parent::getMethod(args.Data()->IntegerValue() >> accessorSetterShift).func, valuePtr);

				const char *message = Status::getError();

				if(message) Nan::ThrowError(message);
			} catch(const std::exception &ex) {
				const char *message = Status::getError();

				if(message == nullptr) message = ex.what();

				Nan::ThrowError(message);
			}
		}
	}
#else
	static void call() {}
#endif // BUILDING_NODE_EXTENSION

};

} // namespace
