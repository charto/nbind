// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Templated static class for each different function call signature exposed by the
// Node.js plugin. Used to pass arguments and return values between C++ and Node.js.
// Everything must be static because the V8 JavaScript engine wants a single
// function pointer to call.

template <typename ReturnType, typename... Args>
class FunctionSignature {

public:

	typedef ReturnType(*FunctionType)(Args...);

	struct FunctionInfo {

		FunctionInfo(const char *name, FunctionType func):name(name), func(func) {}

		const char *name;
		FunctionType func;

	};

	struct SignatureInfo {
		std::vector<struct FunctionInfo> funcVect;
	};

	static FunctionType getFunction(unsigned int num) {
		return(signatureStore().funcVect[num].func);
	}

	static const char *getMethodName(unsigned int num) {
		return(signatureStore().funcVect[num].name);
	}

	static unsigned int addFunction(const char *name, FunctionType func) {
		auto &funcVect = signatureStore().funcVect;

		funcVect.emplace_back(name, func);

		return(funcVect.size() - 1);
	}

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
		>::call(getFunction(args.Data()->IntegerValue()), args);

		char *message = Bindings::getError();

		if(message) return(NanThrowError(message));

		NanReturnValue(BindingType<ReturnType>::toWireType(result));
	}

private:

	static SignatureInfo &signatureStore() {
		static SignatureInfo signatureInfo;
		return(signatureInfo);
	}

};

} // namespace
