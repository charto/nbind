// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles type conversion of C++ standard library types
// to / from JavaScript.

#pragma once

#include <string>
#include <vector>
#include <array>

namespace nbind {

// Array.

template <typename MemberType, size_t size>
struct BindingType<std::array<MemberType, size>> {

	typedef std::array<MemberType, size> type;

	/** Temporary in-memory serialization format for the array,
	  * easily read and written by both C++ and JavaScript. */

	typedef struct {
		/** JavaScript and C++ already know the length based on the type,
		  * but we duplicate it here to use the same wire type as vectors
		  * and share code between them. */
		uint32_t length;
		/** The contents continue past the struct. Use "struct hack"
		  * because C++ doesn't have flexible array members like C. */
		typename BindingType<MemberType>::WireType data[1];
	} *WireType;

	static inline type fromWireType(WireType arg) {
		type val;

		for(uint32_t num = 0; num < size; ++num) {
			val[num] = BindingType<MemberType>::fromWireType(arg->data[num]);
		}

		return(val);
	}

	static inline WireType toWireType(type arg) {
		/** Allocate space for wire type (which includes 1 array member) and
		  * the other size-1 members, in temporary lalloc "stack frame". */
		WireType val = reinterpret_cast<WireType>(NBind::lalloc(sizeof(*val) + (size - 1) * sizeof(*val->data)));

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

	/** Temporary in-memory serialization format for the vector,
	  * easily read and written by both C++ and JavaScript. */

	typedef struct {
		uint32_t length;
		/** The contents continue past the struct. Use "struct hack"
		  * because C++ doesn't have flexible array members like C. */
		typename BindingType<MemberType>::WireType data[1];
	} *WireType;

	static inline type fromWireType(WireType arg) {
		uint32_t size = arg->length;
		type val;

		val.reserve(size);

		for(uint32_t num = 0; num < size; ++num) {
			val.push_back(BindingType<MemberType>::fromWireType(arg->data[num]));
		}

		return(val);
	}

	static inline WireType toWireType(type arg) {
		size_t size = arg.size();
		/** Allocate space for wire type (which includes 1 vector member) and
		  * the other size-1 members, in temporary lalloc "stack frame". */
		WireType val = reinterpret_cast<WireType>(NBind::lalloc(sizeof(*val) + (size - 1) * sizeof(*val->data)));

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

	/** Temporary in-memory serialization format for the string,
	  * easily read and written by both C++ and JavaScript. */

	typedef struct {
		uint32_t length;
		/** The contents continue past the struct. Use "struct hack"
		  * because C++ doesn't have flexible array members like C. */
		char data[1];
	} *WireType;

	static inline Type fromWireType(WireType arg) {
		return(std::string(arg->data, arg->length));
	}

	static inline WireType toWireType(Type arg) {
		size_t length = arg.length();
		/** Allocate space for wire type (which includes 1 character) and
		  * the other size-1 characters, in temporary lalloc "stack frame". */
		WireType val = reinterpret_cast<WireType>(NBind::lalloc(sizeof(*val) + length - 1));

		val->length = length;
		std::copy(arg.begin(), arg.end(), val->data);

		return(val);
	}

};

} // namespace
