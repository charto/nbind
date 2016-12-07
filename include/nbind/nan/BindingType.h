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

	static inline WireType toWireType(ArgType &&arg) {
		return(BindingType<std::shared_ptr<ArgType>>::toWireType(
			// Move construct from stack to heap.
			std::make_shared<ArgType>(std::move(arg))
		));
	}

};

// Object pointer.

template <typename ArgType>
struct BindingType<ArgType *> {

	typedef ArgType *Type;
	typedef typename std::remove_const<ArgType>::type BaseType;

	static inline bool checkType(WireType arg) {
		return(arg->IsObject());
	}

	static inline Type fromWireType(WireType arg) {
		return(BindWrapper<BaseType>::getBound(
			arg->ToObject(),
			std::is_const<ArgType>::value ?
				TypeFlags::isConst :
				TypeFlags::none
		));
	}

	static inline WireType toWireType(Type arg);

};

// Object reference.

template <typename ArgType>
struct BindingType<ArgType &> {

	typedef ArgType &Type;

	static inline bool checkType(WireType arg) {
		return(arg->IsObject());
	}

	static inline Type fromWireType(WireType arg) {
		return(*BindingType<ArgType *>::fromWireType(arg));
	}

	static inline WireType toWireType(Type arg) {
		return(BindingType<ArgType *>::toWireType(&arg));
	}

};

template <typename ArgType>
struct BindingType<std::shared_ptr<ArgType>> {

	typedef std::shared_ptr<ArgType> Type;
	typedef typename std::remove_const<ArgType>::type BaseType;

	static inline bool checkType(WireType arg) {
		return(arg->IsObject());
	}

	static inline Type fromWireType(WireType arg) {
		return(BindWrapper<BaseType>::getShared(
			arg->ToObject(),
			std::is_const<ArgType>::value ?
				TypeFlags::isConst :
				TypeFlags::none
		));
	}

	static inline WireType toWireType(Type &&arg);

};

template <typename ArgType>
struct BindingType<std::unique_ptr<ArgType>> {

	typedef std::unique_ptr<ArgType> Type;
	typedef typename std::remove_const<ArgType>::type BaseType;

	// checkType and fromWireType not supported! C++ objects are wrapped in
	// shared_ptr which would become invalid if passed back as unique_ptr.

	static inline WireType toWireType(Type &&arg);

	// Alternative toWireType definition:
	// return(BindingType<std::shared_ptr<ArgType>>::toWireType(
	// 	std::move(std::shared_ptr<ArgType>(std::move(arg)))
	// ));

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
		return(BindingType<ArgType>::toWireType(std::forward<Type>(arg)));
	}

};

template <typename ArgType>
struct BindingType<ValueType<ArgType>> {

	typedef ArgType Type;

	static inline bool checkType(WireType arg) {
		return(BindingType<ArgType *>::checkType(arg));
	}

	static inline Type fromWireType(WireType arg) noexcept(false);

	static inline WireType toWireType(Type &&arg);

};

// Numeric and boolean types.
// The static cast silences a compiler warning in Visual Studio.

#define DEFINE_NATIVE_BINDING_TYPE(ArgType, checker, decode, jsClass) \
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
		return(arg->checker());                             \
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

} // namespace
