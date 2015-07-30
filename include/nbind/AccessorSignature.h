// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file is very similar to FunctionSignature.h and MethodSignature.h
// so modify them together.

#pragma once

#include "CallableSignature.h"

namespace nbind {

// Getters and setters come in pairs with a single associated metadata value.
// We need to store separate getter and setter ID numbers as metadata
// so they're packed as 16-bit values into a single 32-bit int.
static constexpr unsigned int accessorGetterMask = 0xffff;
static constexpr unsigned int accessorSetterShift = 16;

// Wrapper for all C++ getters and setters with matching class and data types.

template <class Bound, typename ReturnType, typename... Args>
class AccessorSignature : public CallableSignature<AccessorSignature<Bound, ReturnType, Args...>, ReturnType, Args...> {

public:

	typedef struct {
		const char *className;
	} Data;

	typedef ReturnType(Bound::*FunctionType)(Args...);

	typedef CallableSignature<AccessorSignature, ReturnType, Args...> Parent;

	static const char *getClassName() {
		return(Parent::signatureStore().data.className);
	}

	static void setClassName(const char *className) {
		Parent::signatureStore().data.className = className;
	}

	static NAN_GETTER(getter) {
		NanScope();

		v8::Local<v8::Object> targetWrapped = args.This();
		Bound &target = node::ObjectWrap::Unwrap<BindWrapper<Bound>>(targetWrapped)->getBound();

		Bindings::clearError();

		try {
			auto &&result = Caller<
				ReturnType,
				TypeList<>
			>::call(target, Parent::getFunction(args.Data()->IntegerValue() & accessorGetterMask).func, args);

			const char *message = Bindings::getError();

			if(message) return(NanThrowError(message));

			NanReturnValue(BindingType<ReturnType>::toWireType(std::move(result)));
		} catch(const std::exception &ex) {
			const char *message = Bindings::getError();

			if(message == nullptr) message = ex.what();

			return(NanThrowError(message));
		}
	}

	static NAN_SETTER(setter) {
		NanScope();

		v8::Local<v8::Object> targetWrapped = args.This();
		Bound &target = node::ObjectWrap::Unwrap<BindWrapper<Bound>>(targetWrapped)->getBound();

		Bindings::clearError();

		auto *valuePtr = &value;

		if(!AccessorSignature::typesAreValid(valuePtr)) {
			NanThrowTypeError("Type mismatch");
		} else {
			try {
				Parent::CallWrapper::call(target, Parent::getFunction(args.Data()->IntegerValue() >> accessorSetterShift).func, valuePtr);

				const char *message = Bindings::getError();

				if(message) NanThrowError(message);
			} catch(const std::exception &ex) {
				const char *message = Bindings::getError();

				if(message == nullptr) message = ex.what();

				NanThrowError(message);
			}
		}
	}

};

} // namespace
