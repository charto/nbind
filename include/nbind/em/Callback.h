// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles JavaScript callback functions accessible from C++.

#pragma once

#include <emscripten.h>

namespace nbind {

// Wrapper class to specialize call function for different return types,
// since function template partial specialization is forbidden.
template <typename ReturnType>
struct cbCaller {
	typedef typename TypeTransformer<ReturnType>::Binding ReturnBindingType;

	template <typename... Args>
	static ReturnType call(unsigned int num, Args... args);
};

template <typename DefaultReturnType>
class cbWrapper {

public:

	explicit cbWrapper(unsigned int num = 0) : handle(num) {}

	template<typename... Args>
	DefaultReturnType operator()(Args&&... args) {
		return(call<DefaultReturnType>(std::forward<Args>(args)...));
	}

	template <typename ReturnType, typename... Args>
	typename BindingType<ReturnType>::Type call(Args&&... args) const {
		// Restore linear allocator state in RAII style when done.
		PoolRestore restore;

		return(cbCaller<ReturnType>::call(handle.getNum(), std::forward<Args>(args)...));
	}

private:

	External handle;

};

template<> template<typename... Args>
void cbWrapper<void> :: operator()(Args&&... args) {
	call<void>(std::forward<Args>(args)...);
}

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

template <typename... Args>
double callDouble(unsigned int num, Args... args) {
	return(EM_ASM_DOUBLE(
		{return(_nbind.callbackSignatureList[$0].apply(this,arguments));},
		CallbackSignature<double, Args...>::getInstance().getNum(),
		num,
		convertToWire(std::forward<Args>(args))...
	));
}

template <typename ReturnType> template <typename... Args>
ReturnType cbCaller<ReturnType>::call(unsigned int num, Args... args) {
	return(ReturnBindingType::fromWireType(reinterpret_cast<typename ReturnBindingType::WireType>(
		EM_ASM_INT(
			{return(_nbind.callbackSignatureList[$0].apply(this,arguments));},
			CallbackSignature<ReturnType, Args...>::getInstance().getNum(),
			num,
			convertToWire(std::forward<Args>(args))...
		)
	)));
}

template<> struct cbCaller<void> {

	template <typename... Args>
	static void call(unsigned int num, Args... args) {
		EM_ASM_ARGS(
			{return(_nbind.callbackSignatureList[$0].apply(this,arguments));},
			CallbackSignature<void, Args...>::getInstance().getNum(),
			num,
			convertToWire(std::forward<Args>(args))...
		);
	}

};

template<> struct cbCaller<double> {

	template <typename... Args>
	static double call(unsigned int num, Args... args) {
		return(callDouble(num, args...));
	}

};

template<> struct cbCaller<float> {

	template <typename... Args>
	static float call(unsigned int num, Args... args) {
		return(callDouble(num, args...));
	}

};

template<> struct cbCaller<cbOutput::CreateValue> {

	template <typename... Args>
	static int call(unsigned int num, Args... args) {
		return(EM_ASM_ARGS({return(_nbind.callbackSignatureList[$0].apply(this,arguments));},
			CallbackSignature<cbOutput::CreateValue, Args...>::getInstance().getNum(),
			num,
			convertToWire(std::forward<Args>(args))...
		));
	}

};

template <> struct BindingType<const cbFunction &> {

	typedef const cbFunction &Type;

	typedef unsigned int WireType;

};

template <> struct BindingType<cbFunction &> : public BindingType<const cbFunction &> {};

template<typename PolicyList>
struct ArgFromWire<PolicyList, const cbFunction &> {

	explicit ArgFromWire(unsigned int num) : val(num) {}

	inline cbFunction &get(unsigned int num) { return(val); }

	cbFunction val;

};

template<typename PolicyList>
struct ArgFromWire<PolicyList, cbFunction &> {

	explicit ArgFromWire(unsigned int num) : val(num) {}

	inline cbFunction &get(unsigned int num) { return(val); }

	cbFunction val;

};

} // namespace
