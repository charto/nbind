// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file is very similar to FunctionSignature.h and AccessorSignature.h
// so modify them together.

#pragma once

#include "CallableSignature.h"

namespace nbind {

// Wrapper for all C++ methods with matching class, argument and return types.

template <class Bound, typename ReturnType, typename... Args>
class MethodSignature : public CallableSignature<MethodSignature<Bound, ReturnType, Args...>, ReturnType, Args...> {

public:

	typedef struct {
		const char *className;
	} Data;

	typedef ReturnType(Bound::*FunctionType)(Args...);

	typedef CallableSignature<MethodSignature, ReturnType, Args...> Parent;

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

		if(!MethodSignature::typesAreValid(args)) {
			return(NanThrowTypeError("Type mismatch"));
		}

		v8::Local<v8::Object> targetWrapped = args.This();
		Bound &target = node::ObjectWrap::Unwrap<BindWrapper<Bound>>(targetWrapped)->getBound();

		Bindings::clearError();

		try {
			auto result = Parent::CallWrapper::call(target, Parent::getFunction(args.Data()->IntegerValue()).func, args);

			const char *message = Bindings::getError();

			if(message) return(NanThrowError(message));

			NanReturnValue(BindingType<ReturnType>::toWireType(std::move(result)));
		} catch(const std::exception &ex) {
			const char *message = Bindings::getError();

			if(message == nullptr) message = ex.what();

			NanThrowError(message);
		}
	}

};

} // namespace
