// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Templated static class for each possible different method call exposed by the
// Node.js plugin. Used to pass arguments and return values between C++ and Node.js.
// Everything must be static because the V8 JavaScript engine wants a single
// function pointer to call.

template <class Bound, typename ReturnType, typename... Args>
class MethodSignature {

public:

	typedef ReturnType(Bound::*MethodType)(Args...);

	struct MethodInfo {
		MethodInfo(const char *name, MethodType method):name(name), method(method) {}

		const char *name;
		MethodType method;
	};

	struct SignatureInfo {
		const char *className;
		std::vector<struct MethodInfo> methodVect;
	};

	static const char *getClassName() {
		return(signatureStore().className);
	}

	static void setClassName(const char *className) {
		signatureStore().className = className;
	}

	static MethodType getMethod(unsigned int num) {
		return(signatureStore().methodVect[num].method);
	}

	static const char *getMethodName(unsigned int num) {
		return(signatureStore().methodVect[num].name);
	}

	static unsigned int addMethod(const char *name, MethodType method) {
		auto &methodVect = signatureStore().methodVect;

		methodVect.emplace_back(name, method);

		return(methodVect.size() - 1);
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
		>::call(target, getMethod(args.Data()->IntegerValue()), args);

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
