// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include <string>
#include <vector>
#include <array>

namespace nbind {

// Array.

template <typename ArgType, size_t size>
struct BindingType<std::array<ArgType, size>> {

	typedef std::array<ArgType, size> type;

	typedef int WireType;

	static inline type fromWireType(WireType arg) {
		type val;

		return(val);
	}

	static inline WireType toWireType(type arg) {
		return(0);
	}

};

// Vector.

template <typename ArgType>
struct BindingType<std::vector<ArgType>> {

	typedef std::vector<ArgType> type;

	typedef int WireType;

	static inline type fromWireType(WireType arg) {
		type val;

		return(val);
	}

	static inline WireType toWireType(type arg) {
		return(0);
	}

};

// String.

template<> struct BindingType<std::string> {

	typedef std::string Type;

	typedef struct {
		uint32_t length;
		// The string continues past the struct.
		// Use "struct hack" because C++ doesn't have flexible array members like C.
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

} // namespace
