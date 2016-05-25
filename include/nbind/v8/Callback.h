// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles JavaScript callback functions accessible from C++.

#pragma once

namespace nbind {

// cbFunction is a functor that can be called with any number of arguments of any type
// compatible with JavaScript. Types are autodetected from a parameter pack.
// Normally the function returns nothing when called, but it has a templated
// call<ReturnType>() method that accepts the expected return type as a template
// parameter, and handles conversion automatically.

class cbFunction {

	template<size_t Index,typename ArgType>
	friend struct ArgFromWire;

public:

	explicit cbFunction(const v8::Local<v8::Function> &func) : func(func) {}

	cbFunction(const cbFunction &func) : func(func.getJsFunction()) {}

	template<typename... Args>
	void operator()(Args&&... args) {
		call<void>(std::forward<Args>(args)...);
	}

	template <typename ReturnType, typename... Args>
	typename BindingType<ReturnType>::type call(Args... args) {
		v8::Local<v8::Value> argv[] = {
			(BindingType<Args>::toWireType(args))...,
			Nan::Null()
		};
		return(BindingType<ReturnType>::fromWireType(func.Call(sizeof...(Args), argv)));
	}

	template <typename ReturnType, typename... Args>
	typename BindingType<ReturnType>::type callMethod(v8::Local<v8::Object> target, Args... args) {
		v8::Local<v8::Value> argv[] = {
			(BindingType<Args>::toWireType(args))...
		};
		return(BindingType<ReturnType>::fromWireType(func.Call(target, sizeof...(Args), argv)));
	}

	v8::Local<v8::Function> getJsFunction() const { return(func.GetFunction()); }

private:

	Nan::Callback func;

};

class cbOutput {

	template<typename ArgType>
	friend struct BindingType;

public:

	cbOutput(cbFunction &jsConstructor, v8::Local<v8::Value> *output) :
		jsConstructor(jsConstructor), output(output) {}

	// This overload is identical to cbFunction.
	template<typename... Args>
	void operator()(Args&&... args) {
		call<void>(args...);
	}

	template <typename ReturnType, typename... Args>
	void call(Args... args) {
		v8::Local<v8::Value> argv[] = {
			(BindingType<Args>::toWireType(args))...
		};

		*output = jsConstructor.getJsFunction()->NewInstance(sizeof...(Args), argv);
	}

private:

	cbFunction &jsConstructor;
	v8::Local<v8::Value> *output;

};

template <> struct BindingType<cbFunction> {

	typedef const cbFunction type;

	static inline bool checkType(WireType arg) {
		return(arg->IsFunction());
	}

	static inline type fromWireType(WireType arg) {
		return(cbFunction(arg.As<v8::Function>()));
	}

};

template <> struct BindingType<cbFunction &> {

	typedef const cbFunction & type;

	static inline bool checkType(WireType arg) {
		return(arg->IsFunction());
	}

};

// Handle callback functions. They are converted to a functor of type cbFunction,
// which can be called directly from C++ with arguments of any type.

template<size_t Index>
struct ArgFromWire<Index, cbFunction &> {

	template <typename NanArgs>
	ArgFromWire(const NanArgs &args) : val(args[Index].template As<v8::Function>()) {}

	template <typename NanArgs>
	inline cbFunction &get(const NanArgs &args) {
		return(val);
	}

	cbFunction val;

};

} // namespace
