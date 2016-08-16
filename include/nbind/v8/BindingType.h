// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles conversion of C++ primitive types to / from JavaScript
// types in V8 internal representation. Following emscripten conventions,
// the type passed between the two is called WireType.
// Anything from the standard library is instead in BindingStd.h

#pragma once

namespace nbind {

typedef v8::Local<v8::Value> WireType;

// Generic C++ object.

template <typename ArgType> struct BindingType {

	typedef ArgType Type;

	static inline bool checkType(WireType arg) {
		return(BindingType<ArgType *>::checkType(arg));
	}

	static inline Type fromWireType(WireType arg) {
		return(*BindingType<ArgType *>::fromWireType(arg));
	}

	static inline WireType toWireType(ArgType arg) {
		return(BindingType<ArgType *>::toWireType(new ArgType(arg)));
	}

};

// Object pointer.

template <typename ArgType>
struct BindingType<ArgType *> {

	typedef ArgType *Type;

	static inline bool checkType(WireType arg) {
		// TODO: Also check type of object!
		return(arg->IsObject());
	}

	static inline Type fromWireType(WireType arg) {
		v8::Local<v8::Object> argWrapped = arg->ToObject();
		return(node::ObjectWrap::Unwrap<BindWrapper<ArgType>>(argWrapped)->getBound());
	}

	static inline WireType toWireType(Type arg);

};

template <typename ArgType>
struct BindingType<std::shared_ptr<ArgType>> {

	typedef std::shared_ptr<ArgType> Type;

	static inline bool checkType(WireType arg) {
		// TODO: Also check type of object!
		return(arg->IsObject());
	}

	static inline Type fromWireType(WireType arg) {
		v8::Local<v8::Object> argWrapped = arg->ToObject();
		return(node::ObjectWrap::Unwrap<BindWrapper<ArgType>>(argWrapped)->getShared());
	}

	static inline WireType toWireType(Type arg);

};

template <typename ArgType>
struct BindingType<NullableType<ArgType>> {

	typedef typename BindingType<ArgType>::Type Type;

	static inline bool checkType(WireType arg) {
		return(arg->IsNull() || arg->IsUndefined() || BindingType<ArgType>::checkType(arg));
	}

	static inline Type fromWireType(WireType arg) {
		if(arg->IsNull() || arg->IsUndefined()) return(nullptr);
		return(BindingType<ArgType>::fromWireType(arg));
	}

	static inline WireType toWireType(Type arg) {
		if(arg == nullptr) return(Nan::Null());
		return(BindingType<ArgType>::toWireType(arg));
	}

};

// Numeric and boolean types.
// The static cast silences a compiler warning in Visual Studio.

#define DEFINE_NATIVE_BINDING_TYPE(ArgType, check, decode, jsClass) \
template <> struct BindingType<ArgType> {                   \
	typedef ArgType Type;                                   \
	                                                        \
	static inline bool checkType(WireType arg) {            \
		return(true);                                       \
	}                                                       \
	                                                        \
	static inline Type fromWireType(WireType arg) {         \
		return(static_cast<Type>(arg->decode()));           \
	}                                                       \
	                                                        \
	static inline WireType toWireType(Type arg) {           \
		return(Nan::New<jsClass>(arg));                     \
	}                                                       \
};                                                          \
                                                            \
template <> struct BindingType<StrictType<ArgType>> : public BindingType<ArgType> { \
	static inline bool checkType(WireType arg) {            \
		return(arg->check());                               \
	}                                                       \
}

DEFINE_NATIVE_BINDING_TYPE(bool, IsBoolean, BooleanValue, v8::Boolean);

DEFINE_NATIVE_BINDING_TYPE(double, IsNumber, NumberValue, v8::Number);
DEFINE_NATIVE_BINDING_TYPE(float, IsNumber, NumberValue, v8::Number);

DEFINE_NATIVE_BINDING_TYPE(unsigned int, IsNumber, Uint32Value, v8::Uint32);
DEFINE_NATIVE_BINDING_TYPE(unsigned short, IsNumber, Uint32Value, v8::Uint32);
DEFINE_NATIVE_BINDING_TYPE(unsigned char, IsNumber, Uint32Value, v8::Uint32);

DEFINE_NATIVE_BINDING_TYPE(signed int, IsNumber, Int32Value, v8::Int32);
DEFINE_NATIVE_BINDING_TYPE(signed short, IsNumber, Int32Value, v8::Int32);
DEFINE_NATIVE_BINDING_TYPE(signed char, IsNumber, Int32Value, v8::Int32);

DEFINE_NATIVE_BINDING_TYPE(char, IsNumber, Int32Value, v8::Int32);

#define DEFINE_STRING_BINDING_TYPE(ArgType)             \
template <> struct BindingType<ArgType> {               \
	typedef ArgType Type;                               \
	                                                    \
	static inline bool checkType(WireType arg) {        \
		return(true);                                   \
	}                                                   \
	                                                    \
	static inline WireType toWireType(Type arg) {       \
		const char *buf = (arg == nullptr) ? "" : reinterpret_cast<const char *>(arg); \
		return(Nan::New<v8::String>(buf, strlen(buf)).ToLocalChecked()); \
	}                                                   \
};                                                      \
                                                        \
template <> struct BindingType<StrictType<ArgType>> : public BindingType<ArgType> { \
	static inline bool checkType(WireType arg) {        \
		return(arg->IsString());                        \
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

	typedef std::nullptr_t Type;

	static inline Type fromWireType(WireType arg) { return(nullptr); }

	static inline WireType toWireType(Type arg) { return(Nan::Undefined()); }

};

// CheckWire verifies if the type of a JavaScript handle corresponds to a C++ type.

template<typename PolicyList, size_t Index, typename ArgType>
struct CheckWire {

	typedef ArgType Type;
	typedef typename ExecutePolicies<PolicyList>::template Transformed<ArgType>::Type TransformedType;

	template <typename NanArgs>
	static inline bool check(const NanArgs &args) {
		return(BindingType<TransformedType>::checkType(args[Index]));
	}

};

} // namespace
