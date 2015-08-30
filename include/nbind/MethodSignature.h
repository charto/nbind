// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file is very similar to FunctionSignature.h and AccessorSignature.h
// so modify them together.

#pragma once

#include "CallableSignature.h"

namespace nbind {

#ifdef BUILDING_NODE_EXTENSION
template<typename ReturnType> struct MethodResultConverter {

	template <typename Bound>
	static inline auto toWireType(ReturnType &&result, Bound *t) -> typename std::remove_reference<decltype(
		// SFINAE, use this template only if Bound::toJS(ReturnType, cbOutput) exists.
		t->toJS(*(ReturnType *)nullptr, *(cbOutput *)nullptr),
		// Actual return type of this function: WireType (decltype adds a reference, which is removed).
		*(WireType *)nullptr
	)>::type {
		// This function is similar to BindingType<ArgType>::toWireType(ArgType &&arg).

		v8::Local<v8::Value> output = Nan::Undefined();
		cbFunction *jsConstructor = BindClass<typename std::remove_pointer<ReturnType>::type>::getInstance()->getValueConstructorJS();

		if(jsConstructor != nullptr) {
			cbOutput construct(*jsConstructor, &output);

			t->toJS(std::move(result), construct);
		} else {
			// Throw error here?
		}

		return(output);
	}

	// If Bound::toJS(ReturnType, cbOutput) is missing, fall back to ReturnType::toJS(cbOutput).
	static inline WireType toWireType(ReturnType &&result, ...) {
		return(BindingType<ReturnType>::toWireType(std::move(result)));
	}

};

// Convert void return values to undefined.

template<> struct MethodResultConverter<void> {
	static inline WireType toWireType(std::nullptr_t result, ...) {
		return(Nan::Undefined());
	}
};
#endif // BUILDING_NODE_EXTENSION

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

#ifdef BUILDING_NODE_EXTENSION
	static void call(const Nan::FunctionCallbackInfo<v8::Value> &args) {
//	static NAN_METHOD(call) {
		static constexpr decltype(args.Length()) arity = sizeof...(Args);

		if(args.Length() != arity) {
//			printf("Wrong number of arguments to %s.%s: expected %ld, got %d.\n",getClassName(),getMethodName(),arity,args.Length());
			Nan::ThrowError("Wrong number of arguments");
			return;
		}

		if(!MethodSignature::typesAreValid(args)) {
			Nan::ThrowTypeError("Type mismatch");
			return;
		}

		v8::Local<v8::Object> targetWrapped = args.This();
		Bound &target = node::ObjectWrap::Unwrap<BindWrapper<Bound>>(targetWrapped)->getBound();

		Status::clearError();

		try {
			auto result = Parent::CallWrapper::call(target, Parent::getFunction(args.Data()->IntegerValue()).func, args);

			const char *message = Status::getError();

			if(message) {
				Nan::ThrowError(message);
				return;
			}

			args.GetReturnValue().Set(MethodResultConverter<ReturnType>::toWireType(std::move(result), &target));
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
