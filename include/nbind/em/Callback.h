// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles JavaScript callback functions accessible from C++.

#pragma once

#include <emscripten.h>

namespace nbind {

extern "C" {
	extern void _nbind_reference_callback(unsigned int num);
	extern void _nbind_free_callback(unsigned int num);
}

class cbFunction {

public:

	explicit cbFunction(unsigned int num) : num(num) {}

	cbFunction(const cbFunction &func) : num(func.num) {
		_nbind_reference_callback(num);
	}

	~cbFunction() {
		_nbind_free_callback(num);
	}

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
		// Restore linear allocator state in RAII style when done.
		PoolRestore restore;

		return(Caller<ReturnType>::call(num, args...));
	}

	const unsigned int num;
};

class cbOutput {

	template<typename ArgType>
	friend struct BindingType;

public:

	struct CreateValue {};

	cbOutput(cbFunction &jsConstructor) :
		jsConstructor(jsConstructor) {}

	// This overload is identical to cbFunction.
	template<typename... Args>
	void operator()(Args&&... args) {
		call<void>(args...);
	}

	template <typename ReturnType, typename... Args>
	void call(Args... args) {
		jsConstructor.call<CreateValue>(args...);
	}

private:

	cbFunction &jsConstructor;

};

template<typename... Args>
double cbFunction::callDouble(unsigned int num, Args... args) {
	// NOTE: EM_ASM_DOUBLE may have a bug: https://github.com/kripken/emscripten/issues/3770

	return(EM_ASM_DOUBLE({return(_nbind.callbackSignatureList[$0].apply(this,arguments));},
		CallbackSignature<double, Args...>::getInstance().getNum(),
		num,
		BindingType<Args>::toWireType(args)...
	));
}

template <typename ReturnType> template <typename... Args>
ReturnType cbFunction::Caller<ReturnType>::call(unsigned int num, Args... args) {
	return(BindingType<ReturnType>::fromWireType(reinterpret_cast<typename BindingType<ReturnType>::WireType>(
		EM_ASM_INT({return(_nbind.callbackSignatureList[$0].apply(this,arguments));},
			CallbackSignature<ReturnType, Args...>::getInstance().getNum(),
			num,
			BindingType<Args>::toWireType(args)...
		)
	)));
}

template<> struct cbFunction::Caller<void> {

	template <typename... Args>
	static void call(unsigned int num, Args... args) {
		EM_ASM_ARGS({return(_nbind.callbackSignatureList[$0].apply(this,arguments));},
			CallbackSignature<void, Args...>::getInstance().getNum(),
			num,
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

template<> struct cbFunction::Caller<cbOutput::CreateValue> {

	template <typename... Args>
	static cbOutput::CreateValue call(unsigned int num, Args... args) {
		EM_ASM_ARGS({return(_nbind.callbackSignatureList[$0].apply(this,arguments));},
			CallbackSignature<cbOutput::CreateValue, Args...>::getInstance().getNum(),
			num,
			BindingType<Args>::toWireType(args)...
		);

		// Return empty dummy value.
		return(cbOutput::CreateValue());
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
