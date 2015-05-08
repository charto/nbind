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

namespace nbind {

typedef v8::Handle<v8::Value> WireType;

// BindWrapper encapsulates a C++ object created in Node.js.

template <class Bound>
class BindWrapper : public node::ObjectWrap {

public:

	// Pass any constructor arguments to wrapped class.
	template<typename... Args>
	BindWrapper(Args&&... args) : bound(args...) {}

	static NAN_METHOD(create);

//private:

	Bound bound;
};

template <typename ArgType> struct BindingType;

template <typename ArgType>
struct BindingType<ArgType *> {

	typedef ArgType *type;

	static inline ArgType *fromWireType(WireType arg) {
		v8::Local<v8::Object> argWrapped=arg->ToObject();
		return(&node::ObjectWrap::Unwrap<BindWrapper<ArgType>>(argWrapped)->bound);
	}

	static inline WireType toWireType(ArgType arg);

};

#define DEFINE_NATIVE_BINDING_TYPE(ArgType,decode,jsClass)  \
template <> struct BindingType<ArgType> {                   \
	typedef ArgType type;                                   \
	                                                        \
	static inline type fromWireType(WireType arg) {         \
		return(arg->decode());                              \
	}                                                       \
	                                                        \
	static inline WireType toWireType(type arg) {           \
		return(NanNew<jsClass>(arg));                       \
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
	static inline type fromWireType(WireType arg);      \
	                                                    \
	static inline WireType toWireType(type arg) {       \
		auto buf = reinterpret_cast<const char *>(arg); \
		return(NanNew<v8::String>(buf, strlen(buf)));   \
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

DEFINE_STRING_BINDING_TYPE(const unsigned char *);
DEFINE_STRING_BINDING_TYPE(const char *);

// void return values are passed to toWireType as null pointers.

template <> struct BindingType<void> {

	typedef std::nullptr_t type;

	static inline type fromWireType(WireType arg) {return(nullptr);}

	static inline WireType toWireType(std::nullptr_t arg) {return(NanUndefined());}

};

class cbFunction {

	template<size_t Index,typename ArgType>
	friend struct FromWire;

public:
	explicit cbFunction(const v8::Handle<v8::Function> &func) : func(new NanCallback(func)) {}

	template<typename... Args>
	void operator()(Args&&... args) {
		return(call<void>(args...));
	}

	template <typename ReturnType, typename... Args>
	typename BindingType<ReturnType>::type call(Args... args) {
		v8::Handle<v8::Value> argv[] = {
			(BindingType<Args>::toWireType(args))...
		};
		return(BindingType<ReturnType>::fromWireType(func->Call(sizeof...(Args), argv)));
	}

private:

	void destroy() {
		// This cannot be a destructor because cbFunction gets passed by value,
		// so the destructor would get called multiple times.
		delete(func);
	}

	NanCallback *func;

};

template<size_t Index,typename ArgType>
struct FromWire {

	typedef struct inner {

		template <typename NanArgs>
		inner(const NanArgs &args) : val(BindingType<ArgType>::fromWireType(args[Index])) {}

		ArgType get() {return(val);}

		ArgType val;

	} type;

};

template<size_t Index>
struct FromWire<Index, const char *> {

	typedef struct inner {

		template <typename NanArgs>
		inner(const NanArgs &args) : val(args[Index]->ToString()) {}

		const char *get() {return(*val);}

		NanUtf8String val;

	} type;

};

template<size_t Index>
struct FromWire<Index, const unsigned char *> {

	typedef struct inner {

		template <typename NanArgs>
		inner(const NanArgs &args) : val(args[Index]->ToString()) {}

		const unsigned char *get() {return(reinterpret_cast<const unsigned char *>(*val));}

		NanUtf8String val;

	} type;

};

template<size_t Index>
struct FromWire<Index, cbFunction> {

	typedef struct inner {

		template <typename NanArgs>
		inner(const NanArgs &args) : val(args[Index].template As<v8::Function>()) {}

		~inner() {val.destroy();}

		const cbFunction get() {return(val);}

		cbFunction val;

	} type;

};

} // namespace
