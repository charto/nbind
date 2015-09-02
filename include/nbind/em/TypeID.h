// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Type ID system.

typedef const void *TYPEID;

template <typename ArgType>
struct TypeStore {
	// Reserve a single byte of memory to uniquely represent a type.
	// The address of this byte is a unique type-specific constant.
	static char placeholder;

	static constexpr TYPEID get() {
		return(&placeholder);
	}
};

// Linkage for placeholder bytes representing types.
template<typename ArgType>
char TypeStore<ArgType>::placeholder;

template<typename ArgType>
struct TypeID {
	static constexpr TYPEID get() {
		return(TypeStore<ArgType>::get());
	}
};

template <typename ArgType>
static constexpr TYPEID getTypeID() {
	return(TypeStore<ArgType>::get());
}

// Function signature string generation.

template<typename ArgType> constexpr char emMangle();

template<> constexpr char emMangle<int>() {return('i');}
template<> constexpr char emMangle<void>() {return('v');}
template<> constexpr char emMangle<float>() {return('f');}
template<> constexpr char emMangle<double>() {return('d');}

template<typename... Args>
const char* buildEmSignature() {
	static constexpr char signature[] = { emMangle<Args>()..., 0 };

	return(signature);
}

template<typename ArgType> struct EmMangleMap { typedef int type; };
template<> struct EmMangleMap<void> { typedef void type; };
template<> struct EmMangleMap<float> { typedef float type; };
template<> struct EmMangleMap<double> { typedef double type; };

} // namespace
