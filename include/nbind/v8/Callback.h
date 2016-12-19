// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles JavaScript callback functions accessible from C++.

#pragma once

namespace nbind {

// Exception signaling a JavaScript callback has thrown, and an exception is
// now bubbling up the JavaScript stack.

class cbException : public std::exception {};

// cbFunction is a functor that can be called with any number of arguments of any type
// compatible with JavaScript. Types are autodetected from a parameter pack.
// Normally the function returns nothing when called, but it has a templated
// call<ReturnType>() method that accepts the expected return type as a template
// parameter, and handles conversion automatically.

class cbFunction {

	template<typename PolicyList, size_t Index, typename ArgType>
	friend struct ArgFromWire;

public:

	explicit cbFunction(const v8::Local<v8::Function> &func) : func(func) {}

	cbFunction(const cbFunction &func) : func(func.getJsFunction()) {}

	template<typename... Args>
	void operator()(Args&&... args) {
		call<void>(std::move(args)...);
	}

	template <typename ReturnType, typename... Args>
	typename TypeTransformer<ReturnType>::Type call(Args&&... args) {
		v8::Local<v8::Value> argv[] = {
			(convertToWire(std::move(args)))...,
			// Avoid error C2466: cannot allocate an array of constant size 0.
			Nan::Null()
		};

		WireType result = func.Call(sizeof...(Args), argv);

		if(result.IsEmpty()) throw(cbException());

		return(convertFromWire<ReturnType>(result));
	}

	template <typename ReturnType, typename... Args>
	typename TypeTransformer<ReturnType>::Type callMethod(
		v8::Local<v8::Object> target,
		Args&&... args
	) {
		v8::Local<v8::Value> argv[] = {
			(convertToWire(std::move(args)))...,
			// Avoid error C2466: cannot allocate an array of constant size 0.
			Nan::Null()
		};

		WireType result = func.Call(target, sizeof...(Args), argv);

		if(result.IsEmpty()) throw(cbException());

		return(convertFromWire<ReturnType>(result));
	}

	v8::Local<v8::Function> getJsFunction() const { return(func.GetFunction()); }

private:

	Nan::Callback func;

};

template <> struct BindingType<cbFunction> {

	typedef const cbFunction Type;

	static inline bool checkType(WireType arg) {
		return(arg->IsFunction());
	}

	static inline Type fromWireType(WireType arg) {
		return(cbFunction(arg.As<v8::Function>()));
	}

};

template <> struct BindingType<cbFunction &> {

	typedef const cbFunction & Type;

	static inline bool checkType(WireType arg) {
		return(arg->IsFunction());
	}

};

// Handle callback functions. They are converted to a functor of type cbFunction,
// which can be called directly from C++ with arguments of any type.

template<typename PolicyList, size_t Index>
struct ArgFromWire<PolicyList, Index, cbFunction &> {

	template <typename NanArgs>
	ArgFromWire(const NanArgs &args) : val(args[Index].template As<v8::Function>()) {}

	template <typename NanArgs>
	inline cbFunction &get(const NanArgs &args) {
		return(val);
	}

	cbFunction val;

};

template <typename ReturnType, typename... Args>
void cbOutput :: call(Args... args) {
	v8::Local<v8::Value> argv[] = {
		(BindingType<Args>::toWireType(std::forward<Args>(args)))...
	};

	auto result = Nan::NewInstance(jsConstructor.getJsFunction(), sizeof...(Args), argv);

	if(result.IsEmpty()) *output = Nan::Undefined();
	else *output = result.ToLocalChecked();
}

} // namespace
