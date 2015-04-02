// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Templated static class for each possible different method call exposed by the
// Node.js plugin. Used to pass arguments and return values between C++ and Node.js.
// Everything must be static because the V8 JavaScript engine wants a single
// function pointer to call.

// TODO BUG FIX: For every type of method signature there's only one static methodStore pointer!

template <class Bound, typename ReturnType, typename... Args>
class MethodSignature {

public:

	typedef ReturnType(Bound::*MethodType)(Args...);

	struct MethodInfo {
		MethodInfo() {}
		MethodInfo(const char *name, MethodType method):name(name), method(method) {}
		const char *name;
		MethodType method;
	};

	struct SignatureInfo {
		const char *className;
		unsigned int methodCount = 0;
		std::forward_list<struct MethodInfo> methodList;
	};

	static MethodType getMethod() {return(methodStore().method);}
	static const char *getClassName() {return(signatureStore().className);}
	static const char *getMethodName() {return(methodStore().name);}
	static void setClassName(const char *className) {signatureStore().className = className;}
	static void setMethod(const char *name, MethodType method) {
		auto &signature = signatureStore();
		auto &methodInfo = methodStore();

		// printf("COUNT %d\n", signature.methodCount++);
		signature.methodList.emplace_front(name, method);

		methodInfo.name = strdup(name);
		methodInfo.method = method;
	}

	static NAN_METHOD(call) {
		NanScope();
		static constexpr decltype(args.Length()) arity=sizeof...(Args);

		// printf("%p\n", args.Data()->IntegerValue());

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
		>::call(target, getMethod(), args);

		char *message = Bindings::getError();

		if(message) return(NanThrowError(message));

		NanReturnValue(BindingType<ReturnType>::toWireType(result));
	}

private:

	static SignatureInfo &signatureStore() {
		static SignatureInfo signatureInfo;
		return(signatureInfo);
	}

	static MethodInfo &methodStore() {
		static MethodInfo methodInfo;
		return(methodInfo);
	}

};

} // namespace
