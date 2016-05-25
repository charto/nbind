// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include <string>
#include <vector>
#include <array>

namespace nbind {

// Defined in Binding.cc
void *lalloc(size_t size);

// Array.

template <typename MemberType, size_t size>
struct BindingType<std::array<MemberType, size>> {

	typedef std::array<MemberType, size> type;

	typedef struct {
		uint32_t length;
		// The contents continue past the struct.
		// Use "struct hack" because C++ doesn't have flexible array members like C.
		typename BindingType<MemberType>::WireType data[1];
	} *WireType;

	static inline type fromWireType(WireType arg) {
		type val;

		return(val);
	}

	static inline WireType toWireType(type arg) {
		WireType val = reinterpret_cast<WireType>(lalloc(sizeof(*val) + (size - 1) * sizeof(*val->data)));

		val->length = size;

		for(uint32_t num = 0; num < size; ++num) {
			val->data[num] = BindingType<MemberType>::toWireType(arg[num]);
		}

		return(val);
	}

};

// Vector.

template <typename MemberType>
struct BindingType<std::vector<MemberType>> {

	typedef std::vector<MemberType> type;

	typedef struct {
		uint32_t length;
		// The contents continue past the struct.
		// Use "struct hack" because C++ doesn't have flexible array members like C.
		typename BindingType<MemberType>::WireType data[1];
	} *WireType;

	static inline type fromWireType(WireType arg) {
		type val;

		return(val);
	}

	static inline WireType toWireType(type arg) {
		size_t size = arg.size();
		WireType val = reinterpret_cast<WireType>(lalloc(sizeof(*val) + (size - 1) * sizeof(*val->data)));

		val->length = size;

		for(uint32_t num = 0; num < size; ++num) {
			val->data[num] = BindingType<MemberType>::toWireType(arg[num]);
		}

		return(val);
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
		size_t length = arg.length();
		WireType val = reinterpret_cast<WireType>(lalloc(sizeof(*val) + length - 1));

		val->length = length;
		std::copy(arg.begin(), arg.end(), val->data);

		return(val);
	}

};

} // namespace
