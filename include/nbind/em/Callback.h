// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles JavaScript callback functions accessible from C++.

#pragma once

#include <emscripten.h>

namespace nbind {

class cbFunction {

public:

	explicit cbFunction(unsigned int num = 0) : handle(num) {}

	template<typename... Args>
	void operator()(Args&&... args) {
		call<void>(std::forward<Args>(args)...);
	}

	template <typename ReturnType, typename... Args>
	typename BindingType<ReturnType>::Type call(Args&&... args) {
		// Restore linear allocator state in RAII style when done.
		PoolRestore restore;

		return(Caller<ReturnType>::call(handle.getNum(), std::forward<Args>(args)...));
	}

private:

	// Wrapper class to specialize call function for different return types,
	// since function template partial specialization is forbidden.
	template <typename ReturnType>
	struct Caller {
		typedef typename TypeTransformer<ReturnType, PolicyListType<>>::Binding ReturnBindingType;

		template <typename... Args>
		static ReturnType call(unsigned int num, Args... args);
	};

	template<typename... Args>
	static double callDouble(unsigned int num, Args... args);

	External handle;

};

class cbOutput {

	template<typename ArgType>
	friend struct BindingType;

public:

	struct CreateValue {};

	explicit cbOutput(cbFunction &jsConstructor) :
		jsConstructor(jsConstructor), original(*this) {}

	cbOutput(const cbOutput &other) :
		jsConstructor(other.jsConstructor), original(other.original) {}

	cbOutput &operator=(const cbOutput &) = delete;

	template<typename... Args>
	inline int operator()(Args&&... args) {
		original.slot = jsConstructor.call<CreateValue>(args...);

		return(original.slot);
	}

	int getSlot() { return(original.slot); }

private:

	cbFunction &jsConstructor;
	cbOutput &original;
	int slot = 0;

};

template<typename... Args>
double cbFunction::callDouble(unsigned int num, Args... args) {
	return(EM_ASM_DOUBLE(
		{return(_nbind.callbackSignatureList[$0].apply(this,arguments));},
		CallbackSignature<double, Args...>::getInstance().getNum(),
		num,
		TypeTransformer<Args, PolicyListType<>>::Binding::toWireType(std::forward<Args>(args))...
	));
}

template <typename ReturnType> template <typename... Args>
ReturnType cbFunction::Caller<ReturnType>::call(unsigned int num, Args... args) {
	return(ReturnBindingType::fromWireType(reinterpret_cast<typename ReturnBindingType::WireType>(
		EM_ASM_INT(
			{return(_nbind.callbackSignatureList[$0].apply(this,arguments));},
			CallbackSignature<ReturnType, Args...>::getInstance().getNum(),
			num,
			TypeTransformer<Args, PolicyListType<>>::Binding::toWireType(std::forward<Args>(args))...
		)
	)));
}

template<> struct cbFunction::Caller<void> {

	template <typename... Args>
	static void call(unsigned int num, Args... args) {
		EM_ASM_ARGS(
			{return(_nbind.callbackSignatureList[$0].apply(this,arguments));},
			CallbackSignature<void, Args...>::getInstance().getNum(),
			num,
			TypeTransformer<Args, PolicyListType<>>::Binding::toWireType(std::forward<Args>(args))...
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
	static int call(unsigned int num, Args... args) {
		return(EM_ASM_ARGS({return(_nbind.callbackSignatureList[$0].apply(this,arguments));},
			CallbackSignature<cbOutput::CreateValue, Args...>::getInstance().getNum(),
			num,
			TypeTransformer<Args, PolicyListType<>>::Binding::toWireType(std::forward<Args>(args))...
		));
	}

};

template <> struct BindingType<cbFunction &> {

	typedef cbFunction & Type;

	typedef unsigned int WireType;

};

template<typename PolicyList>
struct ArgFromWire<PolicyList, cbFunction &> {

	explicit ArgFromWire(unsigned int num) : val(num) {}

	inline cbFunction &get(unsigned int num) { return(val); }

	cbFunction val;

};

} // namespace
