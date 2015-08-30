// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// Convert between JavaScript types used in the V8 engine and native C++ types.
// Following emscripten conventions, the type passed between the two is called
// WireType.

#pragma once

#include <utility>
#include <cstring>

#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include <nan.h>

namespace nbind {

typedef v8::Local<v8::Value> WireType;
// typedef Nan::MaybeLocal<v8::Value> WireType;

// BindWrapper encapsulates a C++ object created in Node.js.

template <class Bound>
class BindWrapper : public node::ObjectWrap {

public:

	// Pass any constructor arguments to wrapped class.
	template<typename... Args>
	BindWrapper(Args&&... args) : bound(new Bound(args...)) {}

	~BindWrapper() {delete(bound);}

	static void create(const Nan::FunctionCallbackInfo<v8::Value> &args);
//	static NAN_METHOD(create);

	Bound &getBound() {return(*bound);}

private:

	Bound *bound;
};

class cbFunction;

// Generic C++ object.

template <typename ArgType> struct BindingType {

	typedef ArgType type;

	static inline bool checkType(WireType arg) {
		// TODO: Also check type of object!
		return(arg->IsObject());
	}

	static inline ArgType fromWireType(WireType arg);

	static inline WireType toWireType(ArgType arg);

};

// Object pointer.

template <typename ArgType>
struct BindingType<ArgType *> {

	typedef ArgType *type;

	static inline bool checkType(WireType arg) {
		// TODO: Also check type of object!
		return(arg->IsObject());
	}

	static inline type fromWireType(WireType arg) {
		v8::Local<v8::Object> argWrapped = arg->ToObject();
		return(&node::ObjectWrap::Unwrap<BindWrapper<ArgType>>(argWrapped)->getBound());
	}

	static inline WireType toWireType(type arg);

};

#define DEFINE_NATIVE_BINDING_TYPE(ArgType,decode,jsClass)  \
template <> struct BindingType<ArgType> {                   \
	typedef ArgType type;                                   \
	                                                        \
	static inline bool checkType(WireType arg) {            \
		return(true);                                       \
	}                                                       \
	                                                        \
	static inline type fromWireType(WireType arg) {         \
		return(arg->decode());                              \
	}                                                       \
	                                                        \
	static inline WireType toWireType(type arg) {           \
		return(Nan::New<jsClass>(arg));                       \
	}                                                       \
}

DEFINE_NATIVE_BINDING_TYPE(bool,    BooleanValue,v8::Boolean);
DEFINE_NATIVE_BINDING_TYPE(double,  NumberValue, v8::Number);
DEFINE_NATIVE_BINDING_TYPE(float,   NumberValue, v8::Number);
DEFINE_NATIVE_BINDING_TYPE(uint32_t,Uint32Value, v8::Uint32);
DEFINE_NATIVE_BINDING_TYPE(uint16_t,Uint32Value, v8::Uint32);
DEFINE_NATIVE_BINDING_TYPE(uint8_t, Uint32Value, v8::Uint32);
DEFINE_NATIVE_BINDING_TYPE(int32_t, Int32Value,  v8::Int32);
DEFINE_NATIVE_BINDING_TYPE(int16_t, Int32Value,  v8::Int32);
DEFINE_NATIVE_BINDING_TYPE(int8_t,  Int32Value,  v8::Int32);

#define DEFINE_STRING_BINDING_TYPE(ArgType)             \
template <> struct BindingType<ArgType> {               \
	typedef ArgType type;                               \
	                                                    \
	static inline bool checkType(WireType arg) {        \
		return(arg->IsString());                        \
	}                                                   \
	                                                    \
	static inline WireType toWireType(type arg) {       \
		auto buf = reinterpret_cast<const char *>(arg); \
		return(Nan::New<v8::String>(buf, strlen(buf)).ToLocalChecked());   \
	}                                                   \
}

/*
		TODO: functions accepting nbind::Buffer (class remains unimplemented)
		as a parameter could be called like this:

		if(node::Buffer::HasInstance(arg)) {
			v8::Local<v8::Object> buffer = arg->ToObject();
			auto data = reinterpret_cast<const unsigned char *>(node::Buffer::Data(buffer));
			size_t len = node::Buffer::Length(buffer);
			return(nbind::Buffer(data, len));
		}
*/

DEFINE_STRING_BINDING_TYPE(unsigned char *);
DEFINE_STRING_BINDING_TYPE(char *);
DEFINE_STRING_BINDING_TYPE(const unsigned char *);
DEFINE_STRING_BINDING_TYPE(const char *);

// void return values are passed to toWireType as null pointers.

template <> struct BindingType<void> {

	typedef std::nullptr_t type;

	static inline type fromWireType(WireType arg) {return(nullptr);}

	static inline WireType toWireType(type arg) {return(Nan::Undefined());}

};

template <> struct BindingType<cbFunction &> {

	typedef const cbFunction &type;

	static inline bool checkType(WireType arg) {
		return(arg->IsFunction());
	}

};

template <> struct BindingType<v8::Local<v8::Function>> {

	static inline WireType toWireType(v8::Local<v8::Function> arg);

};

// cbFunction is a functor that can be called with any number of arguments of any type
// compatible with JavaScript. Types are autodetected from a parameter pack.
// Normally the function returns nothing when called, but it has a templated
// call<ReturnType>() method that accepts the expected return type as a template
// parameter, and handles conversion automatically.

class cbFunction {

	template<size_t Index,typename ArgType>
	friend struct FromWire;

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
			(BindingType<Args>::toWireType(args))...
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

	v8::Local<v8::Function> getJsFunction() const {return(func.GetFunction());}

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

// CheckWire verifies if the type of a JavaScript handle corresponds to a C++ type.

template<size_t Index,typename ArgType>
struct CheckWire {

	typedef struct {

		template <typename NanArgs>
		static inline bool check(const NanArgs &args) {
			return(BindingType<ArgType>::checkType(args[Index]));
		}

	} type;

};

// FromWire converts JavaScript types into C++ types, usually with BindingType<>::fromWireType
// but some types require additional temporary storage, such as a string converted to C style.
// FromWire is a struct, so wrappers for all objects can be constructed as function arguments,
// and their actual values passed to the called function are returned by the get() function.
// The wrappers go out of scope and are destroyed at the end of the function call.

// Handle most C++ types.

template<size_t Index, typename ArgType>
struct FromWire {

	typedef struct inner {

		template <typename NanArgs>
		inner(const NanArgs &args) {}

		template <typename NanArgs>
		inline ArgType get(const NanArgs &args) noexcept(false) {
			return(BindingType<ArgType>::fromWireType(args[Index]));
		}

	} type;

};

// Handle char pointers, which will receive a C string representation of any JavaScript value.

template<size_t Index>
struct FromWire<Index, const char *> {

	typedef struct inner {

		template <typename NanArgs>
		inner(const NanArgs &args) : val(args[Index]->ToString()) {}

		template <typename NanArgs>
		inline const char *get(const NanArgs &args) {
			return(*val);
		}

		Nan::Utf8String val;

	} type;

};

// Automatically cast char to unsigned if the C++ function expects it.

template<size_t Index>
struct FromWire<Index, const unsigned char *> {

	typedef struct inner {

		template <typename NanArgs>
		inner(const NanArgs &args) : val(args[Index]->ToString()) {}

		template <typename NanArgs>
		inline const unsigned char *get(const NanArgs &args) {
			return(reinterpret_cast<const unsigned char *>(*val));
		}

		Nan::Utf8String val;

	} type;

};

// Handle callback functions. They are converted to a functor of type cbFunction,
// which can be called directly from C++ with arguments of any type.

template<size_t Index>
struct FromWire<Index, cbFunction &> {

	typedef struct inner {

		template <typename NanArgs>
		inner(const NanArgs &args) : val(args[Index].template As<v8::Function>()) {}

		template <typename NanArgs>
		inline cbFunction &get(const NanArgs &args) {
			return(val);
		}

		cbFunction val;

	} type;

};

WireType BindingType<v8::Local<v8::Function>> :: toWireType(v8::Local<v8::Function> arg) {
	return(arg);
}

} // namespace
