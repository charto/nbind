// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// This just calls BindClass<Bound>::getInstance()->getValueConstructorJS();
// It's in a wrapper function defined in BindClass.h to break a circular
// dependency between header files.

template <class Bound>
cbFunction *getValueConstructorJS();

class cbOutput;

template <typename ArgType>
inline ArgType convertFromWire(WireType arg) noexcept(false);

template <typename ArgType>
inline typename BindingType<ArgType>::Type convertFromWire(WireType arg, int dummy) {
	return(BindingType<ArgType>::fromWireType(arg));
}

template <typename ArgType>
inline auto convertFromWire(WireType arg, double dummy) -> typename std::remove_reference<decltype(
	// SFINAE, use this template only if ArgType::toJS(cbOutput) exists
	// (and is const if necessary).
	std::declval<ArgType>().toJS(*(cbOutput *)nullptr),
	// Actual return type of this function: ArgType
	*(ArgType *)nullptr
)>::type {
	return(convertFromWire<ArgType>(arg));
}

template <typename ReturnType>
inline WireType convertToWire(ReturnType result, int dummy) {
	// std::move doesn't work with non-const references.
	return(BindingType<ReturnType>::toWireType(std::forward<ReturnType>(result)));
}

template <typename ReturnType>
inline auto convertToWire(ReturnType result, double dummy) -> typename std::remove_reference<decltype(
	// SFINAE, use this template only if ReturnType::toJS(cbOutput) exists.
	result.toJS(*(cbOutput *)nullptr),
	// Actual return type of this function: WireType (decltype adds a reference, which is removed).
	*(WireType *)nullptr
)>::type {
	v8::Local<v8::Value> output = Nan::Undefined();
	cbFunction *jsConstructor = getValueConstructorJS<
		typename std::remove_const<
			typename std::remove_reference<ReturnType>::type
		>::type
	>();

	if(jsConstructor != nullptr) {
		cbOutput construct(*jsConstructor, &output);

		result.toJS(construct);
	} else {
		throw(std::runtime_error("Value type JavaScript class is missing or not registered"));
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
			// Throw error here?
		}

		return(output);
	}

	// If Bound::toJS(ReturnType, cbOutput) is missing, fall back to ReturnType::toJS(cbOutput).

	template <typename Bound>
	static inline WireType toWireType(ReturnType &&result, Bound &target, double dummy) {
		return(convertToWire<ReturnType>(result, 0.0));
	}

};

} // namespace
