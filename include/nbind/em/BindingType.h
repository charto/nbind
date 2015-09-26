// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

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

#include <string>
#include <vector>

#include <alloca.h>

namespace nbind {

template <typename ArgType> struct BindingType {

	typedef ArgType Type;

	typedef ArgType WireType;

	static inline Type fromWireType(WireType arg) { return(arg); }

	static inline WireType toWireType(Type arg) { return(arg); }

};

template<> struct BindingType<bool> {

	typedef bool Type;

	typedef int WireType;

	static inline Type fromWireType(WireType arg) { return(arg); }

	static inline WireType toWireType(Type arg) { return(arg); }

};

template<> struct BindingType<void> {

	typedef void Type;

	typedef void WireType;

	static inline Type fromWireType() { }

//	static inline WireType toWireType( ) { }
	template <typename... Args>
	static inline WireType toWireType(Args...) { }

};

template<> struct BindingType<std::string> {

	typedef std::string Type;

	typedef struct {
		uint32_t length;
		// The string continues past the struct.
		// Known as the "Struct hack".
		char data[1];
	} *WireType;

	static inline Type fromWireType(WireType arg) {
		return(std::string(arg->data, arg->length));
	}

	static inline WireType toWireType(Type arg) {
		// TODO: JavaScript side and testing for this.
		WireType val = reinterpret_cast<WireType>(alloca(sizeof(*val) + arg.length() - 1));
		val->length = arg.length();
		std::copy(arg.begin(), arg.end(), val->data);
		return(val);
	}

};

template<typename ArgType>
struct ArgFromWire {

	explicit ArgFromWire(typename BindingType<ArgType>::WireType arg) {}

	inline ArgType get(typename BindingType<ArgType>::WireType arg) { return(BindingType<ArgType>::fromWireType(arg)); }

};

template<>
struct ArgFromWire<void> {

	explicit ArgFromWire() {}

	inline void get() {}

};

} // namespace
