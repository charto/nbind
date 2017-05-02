// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles type conversion of C++ standard library types
// to / from JavaScript.

#pragma once

#include <string>
#include <vector>
#include <array>

namespace nbind {

// Shared pointer.

template <typename ArgType>
struct BindingType<std::shared_ptr<ArgType>> {

	typedef std::shared_ptr<ArgType> Type;
	typedef typename BindingType<ArgType>::WireType WireType;

	static inline Type fromWireType(WireType arg) {
		// Hack: JS side sends Type * instead of WireType, since C++ side can
		// easily unwrap the shared_ptr anyway.

		return(*reinterpret_cast<Type *>(arg));
	}

	template <typename SmartType>
	static inline WireType toWireType(SmartType &&arg) {
		WireType val = reinterpret_cast<WireType>(NBind::lalloc(sizeof(*val)));

		val->boundUnsafe = arg.get();
		val->boundShared = new std::shared_ptr<ArgType>(std::move(arg));

		return(val);
	}

};

// Unique pointer.

template <typename ArgType>
struct BindingType<std::unique_ptr<ArgType>> {

	typedef std::unique_ptr<ArgType> Type;
	typedef typename BindingType<ArgType>::WireType WireType;

	static inline WireType toWireType(Type &&arg) {
		return(BindingType<std::shared_ptr<ArgType>>::toWireType(std::move(arg)));
	}

};

// Array.

template <typename MemberType, size_t size>
struct BindingType<std::array<MemberType, size>> {

	typedef typename TypeTransformer<MemberType>::Binding MemberBinding;

	typedef std::array<MemberType, size> Type;

	/** Temporary in-memory serialization format for the array,
	  * easily read and written by both C++ and JavaScript. */

	typedef struct {
		/** JavaScript and C++ already know the length based on the type,
		  * but we duplicate it here to use the same wire type as vectors
		  * and share code between them. */
		uint32_t length;
		/** The contents continue past the struct. Use "struct hack"
		  * because C++ doesn't have flexible array members like C. */
		typename MemberBinding::WireType data[1];
	} *WireType;

	static inline Type fromWireType(WireType arg) {
		Type val;

		for(uint32_t num = 0; num < size; ++num) {
			val[num] = MemberBinding::fromWireType(arg->data[num]);
		}

		return(val);
	}

	static inline WireType toWireType(Type &&arg) {
		/** Allocate space for wire type (which includes 1 array member) and
		  * the other size-1 members, in temporary lalloc "stack frame". */
		WireType val = reinterpret_cast<WireType>(NBind::lalloc(sizeof(*val) + (size - 1) * sizeof(*val->data)));

		val->length = size;

		for(uint32_t num = 0; num < size; ++num) {
			val->data[num] = MemberBinding::toWireType(std::forward<MemberType>(arg[num]));
		}

		return(val);
	}

};

// Vector.

template <typename MemberType>
struct BindingType<std::vector<MemberType>> {

	typedef typename TypeTransformer<MemberType>::Binding MemberBinding;

	typedef std::vector<MemberType> Type;

	/** Temporary in-memory serialization format for the vector,
	  * easily read and written by both C++ and JavaScript. */

	typedef struct {
		uint32_t length;
		/** The contents continue past the struct. Use "struct hack"
		  * because C++ doesn't have flexible array members like C. */
		typename MemberBinding::WireType data[1];
	} *WireType;

	static inline Type fromWireType(WireType arg) {
		uint32_t size = arg->length;
		Type val;

		val.reserve(size);

		for(uint32_t num = 0; num < size; ++num) {
			val.push_back(MemberBinding::fromWireType(arg->data[num]));
		}

		return(val);
	}

	static inline WireType toWireType(Type &&arg) {
		size_t size = arg.size();
		/** Allocate space for wire type (which includes 1 vector member) and
		  * the other size-1 members, in temporary lalloc "stack frame". */
		WireType val = reinterpret_cast<WireType>(NBind::lalloc(sizeof(*val) + (size - 1) * sizeof(*val->data)));

		val->length = size;

		for(uint32_t num = 0; num < size; ++num) {
			val->data[num] = MemberBinding::toWireType(std::forward<MemberType>(arg[num]));
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

template <> struct BindingType<StrictType<std::string>> : public BindingType<std::string> {};

// String reference.

template<> struct BindingType<const std::string &> {

	typedef std::string Type;

	typedef struct {
		uint32_t length;
		char data[1];
	} *WireType;

};

template <> struct BindingType<StrictType<const std::string &>> : public BindingType<const std::string &> {};

template<typename PolicyList>
struct ArgFromWire<PolicyList, const std::string &> {

	typedef typename BindingType<const std::string &>::WireType WireType;

	explicit ArgFromWire(WireType arg) : val(arg->data, arg->length) {}

	inline const std::string &get(WireType arg) const {
		return(val);
	}

	// RAII style storage for the string data.

	std::string val;

};

} // namespace
