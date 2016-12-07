// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Calls BindClass<Bound>::getInstance()->getValueConstructorJS();
// We don't want to depend on BindClass.h here.

template <class Bound>
cbFunction *getValueConstructorJS();

// Send value object to JavaScript using toJS method of its C++ class.
// It was received by value, so it can be moved.

template <typename ArgType>
inline WireType BindingType<ValueType<ArgType>>::toWireType(ArgType &&arg) {
	v8::Local<v8::Value> output = Nan::Undefined();
	cbFunction *jsConstructor = getValueConstructorJS<
		typename std::remove_const<ArgType>::type
	>();

	if(jsConstructor != nullptr) {
		cbOutput construct(*jsConstructor, &output);

		arg.toJS(construct);
	} else {
		// Value type JavaScript class is missing or not registered.
		return(BindingType<ArgType *>::toWireType(new ArgType(std::move(arg))));
	}

	return(output);
}

// Handle value types as return values.
// This converter allows overriding the return type's toJS function
// with the wrapped object's toJS function.

template<typename ReturnType> struct MethodResultConverter {

	// Call the toJS method of a returned C++ object, to convert it into a JavaScript object.
	// This is used when a C++ function is called from JavaScript.
	// A functor capable of calling the correct JavaScript constructor is passed to toJS,
	// which must call the functor with arguments in the correct order.
	// The functor calls the JavaScript constructor and writes a pointer to the resulting object
	// directly into a local handle called "output" which is returned to JavaScript.

	template <typename Bound>
	static inline auto toWireType(ReturnType &&result, Bound &target, int dummy) -> typename std::remove_reference<decltype(
		// SFINAE, use this template only if Bound::toJS(ReturnType, cbOutput) exists.
		target.toJS(std::declval<ReturnType>(), *(cbOutput *)nullptr),
		// Actual return type of this function: WireType (decltype adds a reference, which is removed).
		*(WireType *)nullptr
	)>::type {
		v8::Local<v8::Value> output = Nan::Undefined();
		cbFunction *jsConstructor = getValueConstructorJS<ReturnType>();

		if(jsConstructor != nullptr) {
			cbOutput construct(*jsConstructor, &output);

			target.toJS(std::move(result), construct);
		} else {
			return(BindingType<ReturnType *>::toWireType(new ReturnType(std::move(result))));
		}

		return(output);
	}

	// If Bound::toJS(ReturnType, cbOutput) is missing
	// (bound may not even be a class), fall back to ReturnType::toJS(cbOutput).

	template <typename Bound>
	static inline WireType toWireType(ReturnType &&result, Bound &target, double dummy) {
		return(convertToWire(std::forward<ReturnType>(result)));
	}

};

} // namespace
