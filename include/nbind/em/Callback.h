// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include <emscripten.h>

namespace nbind {

class cbFunction {

public:

	explicit cbFunction(unsigned int num) : num(num) {}

	// Wrapper class to specialize call function for different return types,
	// since function template partial specialization is forbidden.
	template <typename ReturnType>
	struct Caller {
		template <typename... Args>
		static ReturnType call(unsigned int num, Args... args);
	};

	template<typename... Args>
	static double callDouble(unsigned int num, Args... args);

	template<typename... Args>
	void operator()(Args&&... args) {
		call<void>(std::forward<Args>(args)...);
	}

	template <typename ReturnType, typename... Args>
	ReturnType call(Args... args) {
		return(Caller<ReturnType>::call(num, args...));
	}

	unsigned int num;
};

template<typename... Args>
double cbFunction::callDouble(unsigned int num, Args... args) {
	static CallbackSignature<double, Args...> signature;

	// NOTE: EM_ASM_DOUBLE may have a bug: https://github.com/kripken/emscripten/issues/3770

	return(EM_ASM_DOUBLE({return(_nbind.callCallback.apply(this,arguments));},
		num,
		signature.getNum(),
		BindingType<Args>::toWireType(args)...
	));
}

template <typename ReturnType> template <typename... Args>
ReturnType cbFunction::Caller<ReturnType>::call(unsigned int num, Args... args) {
	static CallbackSignature<ReturnType, Args...> signature;

	return(BindingType<ReturnType>::fromWireType(reinterpret_cast<typename BindingType<ReturnType>::WireType>(
		EM_ASM_INT({return(_nbind.callCallback.apply(this,arguments));},
			num,
			signature.getNum(),
			BindingType<Args>::toWireType(args)...
		)
	)));
}

template<> struct cbFunction::Caller<void> {

	template <typename... Args>
	static void call(unsigned int num, Args... args) {
		static CallbackSignature<void, Args...> signature;

		EM_ASM_ARGS({return(_nbind.callCallback.apply(this,arguments));},
			num,
			signature.getNum(),
			BindingType<Args>::toWireType(args)...
		);
	}

};

template<> struct cbFunction::Caller<double> {

	template <typename... Args>
	static double call(unsigned int num, Args... args) {
		return(cbFunction::callDouble(num, args...));
	}

};

template<> struct cbFunction::Caller<float> {

	template <typename... Args>
	static float call(unsigned int num, Args... args) {
		return(cbFunction::callDouble(num, args...));
	}

};

template <> struct BindingType<cbFunction &> {

	typedef cbFunction & Type;

	typedef unsigned int WireType;

//	static inline Type fromWireType(WireType arg);

//	static inline WireType toWireType(Type arg);

};

template<>
struct ArgFromWire<cbFunction &> {

	explicit ArgFromWire(unsigned int num) : val(num) {}

	inline cbFunction &get(unsigned int num) { return(val); }

	cbFunction val;

};

} // namespace
