// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file is very similar to MethodSignature.h and AccessorSignature.h
// so modify them together.

#pragma once

#include "BaseSignature.h"

namespace nbind {

// Wrapper for all C++ functions with matching argument and return types.

template <typename ReturnType, typename... Args>
class FunctionSignature : public TemplatedBaseSignature<FunctionSignature<ReturnType, Args...>, ReturnType, Args...> {

public:

	typedef ReturnType(*MethodType)(Args...);

	typedef TemplatedBaseSignature<FunctionSignature, ReturnType, Args...> Parent;

#ifdef BUILDING_NODE_EXTENSION
	static void call(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		static constexpr decltype(args.Length()) arity = sizeof...(Args);

		// TODO: For function overloading support, this test needs to be moved elsewhere.

		if(args.Length() != arity) {
			Nan::ThrowError("Wrong number of arguments");
			return;
		}

		if(!FunctionSignature::typesAreValid(args)) {
			Nan::ThrowTypeError("Type mismatch");
			return;
		}

		Status::clearError();

		try {
			auto result = Parent::CallWrapper::call(Parent::getMethod(args.Data()->IntegerValue()).func, args);

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
