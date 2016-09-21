// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles conversion of C++ primitive types to / from JavaScript.
// Following emscripten conventions, the type passed between the two is called
// WireType.
// Anything from the standard library is instead in BindingStd.h

/*
This file is very similar to wire.h in Embind, and serves the same purpose.
The following conditions apply to any identical parts:

Copyright (c) 2010-2014 Emscripten authors, see EMSCRIPTEN-AUTHORS file.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace nbind {

// Generic C++ object.

template <typename ArgType> struct BindingType {

	typedef ArgType Type;
	typedef struct {
		std::shared_ptr<ArgType> *boundShared;
		ArgType *boundUnsafe;
	} *WireType;

	static inline Type fromWireType(WireType arg) {
		// Hack: JS side sends Type * instead of WireType, since C++ side can
		// easily unwrap the shared_ptr anyway.

		return(*reinterpret_cast<Type *>(arg));
	}

	static inline WireType toWireType(ArgType &&arg) {
		WireType val = reinterpret_cast<WireType>(NBind::lalloc(sizeof(*val)));

		val->boundUnsafe = new ArgType(std::move(arg));
		val->boundShared = new std::shared_ptr<ArgType>(val->boundUnsafe);

		return(val);
	}

	static inline WireType toWireType(const ArgType &arg) {
		WireType val = reinterpret_cast<WireType>(NBind::lalloc(sizeof(*val)));

		val->boundUnsafe = new ArgType(arg);
		val->boundShared = new std::shared_ptr<ArgType>(val->boundUnsafe);

		return(val);
	}

};

// Object pointer.

template <typename ArgType>
struct BindingType<ArgType *> {

	typedef ArgType *Type;
	typedef ArgType *WireType;

	// checkType is not called on Emscripten target.
	// static inline bool checkType(WireType arg) { return(arg != nullptr); }

	static inline Type fromWireType(WireType arg) { return(arg); }
	static inline WireType toWireType(Type arg) { return(arg); }

};

// Object reference.

template <typename ArgType>
struct BindingType<ArgType &> {

	typedef ArgType &Type;
	typedef ArgType *WireType;

	static inline Type fromWireType(WireType arg) { return(*arg); }
	static inline WireType toWireType(Type arg) { return(&arg); }

};

// Object rvalue reference.

template <typename ArgType>
struct BindingType<ArgType &&> {

	typedef ArgType &&Type;
	typedef ArgType *WireType;

	static inline Type fromWireType(WireType arg) { return(*arg); }
	static inline WireType toWireType(Type arg) { return(&arg); }

};

// Nullable type, just calls the non-nullable version.
// Functional differences are on JS side.

template <typename ArgType>
struct BindingType<NullableType<ArgType>> {

	typedef typename BindingType<ArgType>::Type Type;
	typedef typename BindingType<ArgType>::WireType WireType;

	static inline Type fromWireType(WireType arg) { return(arg); }
	static inline WireType toWireType(Type arg) { return(arg); }

};

template <typename ArgType>
struct BindingType<ValueType<ArgType>> {

	typedef ArgType Type;
	typedef typename std::remove_reference<ArgType>::type ObjType;

	// Pointer or offset (times 2 plus 1 to distinguish from pointers)
	// to a list of constructed objects on the JavaScript side.

	typedef typename BindingType<ObjType>::WireType WireType;

	static inline Type fromWireType(WireType arg);
	static inline WireType toWireType(Type &&arg);

};

#define DEFINE_NATIVE_BINDING_TYPE(ArgType, ArgWireType) \
template <> struct BindingType<ArgType> { \
	typedef ArgType Type; \
	typedef ArgWireType WireType; \
	static inline Type fromWireType(WireType arg) { return(arg); } \
	static inline WireType toWireType(Type arg) { return(arg); } \
}; \
\
template <> struct BindingType<StrictType<ArgType>> : public BindingType<ArgType> {}

DEFINE_NATIVE_BINDING_TYPE(double, double);
DEFINE_NATIVE_BINDING_TYPE(float, double);

DEFINE_NATIVE_BINDING_TYPE(unsigned int, int);
DEFINE_NATIVE_BINDING_TYPE(unsigned short, int);
DEFINE_NATIVE_BINDING_TYPE(unsigned char, int);

DEFINE_NATIVE_BINDING_TYPE(int, int);
DEFINE_NATIVE_BINDING_TYPE(short, int);
DEFINE_NATIVE_BINDING_TYPE(char, int);

DEFINE_NATIVE_BINDING_TYPE(unsigned char *, unsigned char *);
DEFINE_NATIVE_BINDING_TYPE(char *, char *);
DEFINE_NATIVE_BINDING_TYPE(const unsigned char *, const unsigned char *);
DEFINE_NATIVE_BINDING_TYPE(const char *, const char *);

template<> struct BindingType<bool> {

	typedef bool Type;

	typedef int WireType;

	static inline Type fromWireType(WireType arg) { return(arg != 0); }

	static inline WireType toWireType(Type arg) { return(arg); }

};

template <> struct BindingType<StrictType<bool>> : public BindingType<bool> {};

template<> struct BindingType<void> {

	typedef void Type;

	typedef void WireType;

	static inline Type fromWireType() { }

	template <typename... Args>
	static inline WireType toWireType(Args...) { }

};

} // namespace
