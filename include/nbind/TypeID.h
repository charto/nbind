// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Type ID system, based on Embind.

typedef const void *TYPEID;

// Type ID generator.

template<typename ArgType>
struct Typer {
	// Reserve a single byte of memory to uniquely represent a type.
	// The address of this byte is a unique type-specific constant.
	static const struct SpecType {
		const unsigned char placeholderFlag;
	} spec;

	static NBIND_CONSTEXPR TYPEID makeID() {
		return(&spec.placeholderFlag);
	}
};

// Linkage for placeholder bytes representing types.
template<typename ArgType>
const typename Typer<ArgType>::SpecType Typer<ArgType>::spec = {0};

// Type description helpers.

template <typename ArgType>
struct isChar {
	static constexpr bool value = false;
};

template <>
struct isChar<char> {
	static constexpr bool value = true;
};

// Convert a list of types in the template argument into an array of type IDs.

template<typename... Args>
const TYPEID *listTypes() {
	static NBIND_CONSTEXPR TYPEID typeList[] = { Typer<Args>::makeID()... };
	return(typeList);
}

} // namespace
