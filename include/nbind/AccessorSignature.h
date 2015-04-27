// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file is very similar to FunctionSignature.h and MethodSignature.h
// so modify them together or try to remove any duplication.

#pragma once

#include "CallableSignature.h"

namespace nbind {

// Getters and setters come in pairs with a single associated metadata value.
// We need to store separate getter and setter ID numbers as metadata
// so they're packed as 16-bit values into a single 32-bit int.
static constexpr unsigned int accessorGetterMask = 0xffff;
static constexpr unsigned int accessorSetterShift = 16;

struct AccessorSignatureData {
	const char *className;
};

// Wrapper for all C++ getters and setters with matching class and data types.

template <class Bound, typename ReturnType, typename... Args>
class AccessorSignature : public CallableSignature<ReturnType (Bound::*)(Args...), AccessorSignatureData> {

public:

	typedef CallableSignature<ReturnType (Bound::*)(Args...), AccessorSignatureData> Parent;

	static const char *getClassName() {
		return(Parent::signatureStore().data.className);
	}

	static void setClassName(const char *className) {
		Parent::signatureStore().data.className = className;
	}

	static NAN_GETTER(getter) {
		NanScope();

		v8::Local<v8::Object> targetWrapped = args.This();
		Bound &target = node::ObjectWrap::Unwrap<BindWrapper<Bound>>(targetWrapped)->bound;

		Bindings::clearError();

		auto &&result = Caller<
			ReturnType,
			TypeList<>
		>::call(target, Parent::getFunction(args.Data()->IntegerValue() & accessorGetterMask), args);

		char *message = Bindings::getError();

		if(message) return(NanThrowError(message));

		NanReturnValue(BindingType<ReturnType>::toWireType(result));
	}

	static NAN_SETTER(setter) {
		NanScope();

		v8::Local<v8::Object> targetWrapped = args.This();
		Bound &target = node::ObjectWrap::Unwrap<BindWrapper<Bound>>(targetWrapped)->bound;

		Bindings::clearError();

		auto *valuePtr = &value;

		// TODO: Check argument type!

		Caller<
			ReturnType,
			typename emscripten::internal::MapWithIndex<
				TypeList,
				FromWire,
				Args...
			>::type
		>::call(target, Parent::getFunction(args.Data()->IntegerValue() >> accessorSetterShift), valuePtr);

		char *message = Bindings::getError();

		if(message) NanThrowError(message);
	}

};

} // namespace
