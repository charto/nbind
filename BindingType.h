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
typedef v8::Local<v8::Value> WireTypeLocal;

// BindWrapper encapsulates a C++ object created in Node.js.

template <class Bound>
class BindWrapper : public node::ObjectWrap {

public:

	// Pass any constructor arguments to wrapped class.
	template<typename... Args>
		BindWrapper(Args&&... args):bound(args...) {}

	static NAN_METHOD(create);

//private:

	Bound bound;
};

template <typename ArgType> struct BindingType;

template <typename ArgType>
struct BindingType<ArgType *> {
	static inline ArgType *fromWireType(WireTypeLocal arg) {
		v8::Local<v8::Object> argWrapped=arg->ToObject();
		return(&node::ObjectWrap::Unwrap<BindWrapper<ArgType>>(argWrapped)->bound);
	}

	static inline WireType toWireType(ArgType arg);
};

#define DEFINE_NATIVE_BINDING_TYPE(type,decode,jsClass)         \
template <> struct BindingType<type> {                          \
	static inline type fromWireType(WireTypeLocal arg) {        \
		return(arg->decode());                                  \
	}                                                           \
	                                                            \
	static inline WireType toWireType(type arg) {               \
		return(NanNew<jsClass>(arg));                           \
	}                                                           \
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

#define DEFINE_STRING_BINDING_TYPE(type)										\
template <> struct BindingType<type> {											\
	static inline type fromWireType(WireTypeLocal arg) {						\
		return(reinterpret_cast<type>(*NanUtf8String(arg->ToString())));		\
	}																			\
																				\
	static inline WireType toWireType(type arg) {								\
		auto buf = reinterpret_cast<const char *>(arg);							\
		return(NanNew<v8::String>(buf, strlen(buf)));							\
	}																			\
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
	static inline std::nullptr_t fromWireType(WireTypeLocal arg) {return(nullptr);}
	static inline WireType toWireType(std::nullptr_t arg) {return(NanUndefined());}
};

template<size_t Index,typename ArgType>
struct FromWire {

	typedef struct {

		template <typename NanArgs>
		static ArgType get(const NanArgs &args) {
			return(BindingType<ArgType>::fromWireType(args[Index]));
		}
	} type;

};

} // namespace
