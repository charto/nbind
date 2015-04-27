// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file is very similar to FunctionSignature.h and AccessorSignature.h
// so modify them together or try to remove any duplication.

#pragma once

#include "CallableSignature.h"

namespace nbind {

// Wrapper for all C++ methods with matching class, argument and return types.

struct MethodSignatureData {
	const char *className;
};

template <class Bound, typename ReturnType, typename... Args>
class MethodSignature : public CallableSignature<ReturnType (Bound::*)(Args...), MethodSignatureData> {

public:

	typedef CallableSignature<ReturnType (Bound::*)(Args...), MethodSignatureData> Parent;

	static const char *getClassName() {
		return(Parent::signatureStore().data.className);
	}

	static void setClassName(const char *className) {
		Parent::signatureStore().data.className = className;
	}

	static NAN_METHOD(call) {
		static constexpr decltype(args.Length()) arity = sizeof...(Args);

		NanScope();

		if(args.Length() != arity) {
//			printf("Wrong number of arguments to %s.%s: expected %ld, got %d.\n",getClassName(),getMethodName(),arity,args.Length());
			return(NanThrowError("Wrong number of arguments"));
		}

		v8::Local<v8::Object> targetWrapped = args.This();
		Bound &target = node::ObjectWrap::Unwrap<BindWrapper<Bound>>(targetWrapped)->bound;

		Bindings::clearError();

		// TODO: Check argument types!

		auto &&result = Caller<
			ReturnType,
			typename emscripten::internal::MapWithIndex<
				TypeList,
				FromWire,
				Args...
			>::type
		>::call(target, Parent::getFunction(args.Data()->IntegerValue()), args);

		char *message = Bindings::getError();

		if(message) return(NanThrowError(message));

		NanReturnValue(BindingType<ReturnType>::toWireType(result));
	}

};

} // namespace
