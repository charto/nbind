// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file is very similar to MethodSignature.h and AccessorSignature.h
// so modify them together.

#pragma once

#include "CallableSignature.h"

namespace nbind {

// Wrapper for all C++ functions with matching argument and return types.

template <typename ReturnType, typename... Args>
class FunctionSignature : public CallableSignature<FunctionSignature<ReturnType, Args...>, ReturnType, Args...> {

public:

	typedef ReturnType(*FunctionType)(Args...);

	typedef CallableSignature<FunctionSignature, ReturnType, Args...> Parent;

#ifdef BUILDING_NODE_EXTENSION
	static void call(const Nan::FunctionCallbackInfo<v8::Value> &args) {
//	static NAN_METHOD(call) {
		static constexpr decltype(args.Length()) arity = sizeof...(Args);

		if(args.Length() != arity) {
//			printf("Wrong number of arguments to %s.%s: expected %ld, got %d.\n",getClassName(),getMethodName(),arity,args.Length());
			Nan::ThrowError("Wrong number of arguments");
			return;
		}

		if(!FunctionSignature::typesAreValid(args)) {
			Nan::ThrowTypeError("Type mismatch");
			return;
		}

		Status::clearError();

		try {
			auto result = Parent::CallWrapper::call(Parent::getFunction(args.Data()->IntegerValue()).func, args);

			const char *message = Status::getError();

			if(message != nullptr) {
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
