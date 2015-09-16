// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file is very similar to FunctionSignature.h and AccessorSignature.h
// so modify them together.

#pragma once

#include "BaseSignature.h"

namespace nbind {

#if defined(BUILDING_NODE_EXTENSION)

// Handle value types as return values.

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
class MethodSignature : public TemplatedBaseSignature<MethodSignature<Bound, ReturnType, Args...>, ReturnType, Args...> {

public:

	typedef ReturnType(Bound::*MethodType)(Args...);

	typedef TemplatedBaseSignature<MethodSignature, ReturnType, Args...> Parent;

	static constexpr auto typeExpr = BaseSignature::Type::method;

#if defined(BUILDING_NODE_EXTENSION)

	template <typename V8Args, typename NanArgs>
	static bool callInner(V8Args &args, NanArgs &nanArgs, Bound *target) {
		auto result = Parent::CallWrapper::call(
			*target,
			Parent::getMethod(nanArgs.Data()->IntegerValue() & signatureMemberMask).func,
			args
		);

		if(Status::getError() != nullptr) return(false);

		nanArgs.GetReturnValue().Set(MethodResultConverter<ReturnType>::toWireType(std::move(result), target));
		return(true);
	}

	static void call(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		Parent::template callInnerSafely<Bound>(args, args);
	}

#elif defined(EMSCRIPTEN)

	// Args are wire types! They must be received by value.

	static ReturnType call(uint32_t num, Bound *target, Args... args) {
		auto method = Parent::getMethod(num).func;
		return((target->*method)(args...));
	}

#endif // BUILDING_NODE_EXTENSION, EMSCRIPTEN

};

} // namespace
