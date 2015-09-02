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
class GetterSignature : public BaseSignature<GetterSignature<Bound, ReturnType, Args...>, ReturnType, Args...> {

public:

	typedef ReturnType(Bound::*MethodType)(Args...);

	typedef BaseSignature<GetterSignature, ReturnType, Args...> Parent;

#ifdef BUILDING_NODE_EXTENSION
	static void call(v8::Local<v8::String> property, const Nan::PropertyCallbackInfo<v8::Value> &args) {

		v8::Local<v8::Object> targetWrapped = args.This();
		Bound &target = node::ObjectWrap::Unwrap<BindWrapper<Bound>>(targetWrapped)->getBound();

		Status::clearError();

		try {
			auto &&result = Caller<
				ReturnType,
				TypeList<>
			>::call(target, Parent::getMethod(args.Data()->IntegerValue() & accessorGetterMask).func, args);

			const char *message = Status::getError();

			if(message) {
				Nan::ThrowError(message);
				return;
			}

			args.GetReturnValue().Set(BindingType<ReturnType>::toWireType(std::move(result)));
		} catch(const std::exception &ex) {
			const char *message = Status::getError();

			if(message == nullptr) message = ex.what();

			Nan::ThrowError(message);
			return;
		}
	}
#else
	static void call() {}
#endif // BUILDING_NODE_EXTENSION

};

} // namespace
