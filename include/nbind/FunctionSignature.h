// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file is very similar to MethodSignature.h and AccessorSignature.h
// so modify them together or try to remove any duplication.

#pragma once

#include "CallableSignature.h"

namespace nbind {

struct FunctionSignatureData {};

// Wrapper for all C++ functions with matching argument and return types.

template <typename ReturnType, typename... Args>
class FunctionSignature : public CallableSignature<ReturnType (*)(Args...), FunctionSignatureData> {

public:

	typedef CallableSignature<ReturnType (*)(Args...), FunctionSignatureData> Parent;

	static NAN_METHOD(call) {
		static constexpr decltype(args.Length()) arity = sizeof...(Args);

		NanScope();

		if(args.Length() != arity) {
//			printf("Wrong number of arguments to %s.%s: expected %ld, got %d.\n",getClassName(),getMethodName(),arity,args.Length());
			return(NanThrowError("Wrong number of arguments"));
		}

		Bindings::clearError();

		// TODO: Check argument types!

		auto &&result = Caller<
			ReturnType,
			typename emscripten::internal::MapWithIndex<
				TypeList,
				FromWire,
				Args...
			>::type
		>::call(Parent::getFunction(args.Data()->IntegerValue()), args);

		char *message = Bindings::getError();

		if(message) return(NanThrowError(message));

		NanReturnValue(BindingType<ReturnType>::toWireType(result));
	}

};

} // namespace
