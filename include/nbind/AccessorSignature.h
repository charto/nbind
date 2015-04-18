// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file is very similar to FunctionSignature.h and MethodSignature.h
// so modify them together or try to remove any duplication.

#pragma once

namespace nbind {

// Templated static class for each different method call signature exposed by the
// Node.js plugin. Used to pass arguments and return values between C++ and Node.js.
// Everything must be static because the V8 JavaScript engine wants a single
// function pointer to call.

template <class Bound, typename ReturnType, typename... Args>
class AccessorSignature {

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

	static NAN_GETTER(getter) {
		NanScope();

		v8::Local<v8::Object> targetWrapped = args.This();
		Bound &target = node::ObjectWrap::Unwrap<BindWrapper<Bound>>(targetWrapped)->bound;

		Bindings::clearError();

		auto &&result = Caller<
			ReturnType,
			TypeList<>
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
