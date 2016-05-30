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
		const char placeholderFlag;
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

// This function takes a list of primitive types as the template argument.
// It defines 3 arrays with information about them: type IDs, sizes and properties of the types.
// For example uint32_t, float64_t and const char * can be distinguished by their size and the property flags
// char, const, pointer, float and unsigned.
// Char is not treated as a simple int8_t because as a const pointer it can also identify a string literal.

template <typename... Args>
const void **defineTypes() {
	static TYPEID typeList[] = { Typer<Args>::makeID()..., nullptr };
	static const uint32_t sizeList[] = { sizeof(typename std::remove_pointer<Args>::type)... };
	static const uint8_t flagList[] = { (
		isChar<typename std::remove_pointer<Args>::type>::value * 16 |
		std::is_const<typename std::remove_pointer<Args>::type>::value * 8 |
		std::is_pointer<Args>::value * 4 |
		// Type is floating point?
		((typename std::remove_pointer<Args>::type)1/2 != 0) * 2 |
		// Type is unsigned?
		((typename std::remove_pointer<Args>::type)-1 >= 0)
	)... };

	static const void *data[] = { reinterpret_cast<void *>(sizeof...(Args)), typeList, sizeList, flagList };
	return(data);
}

// Convert a list of types in the template argument into an array of type IDs.

template<typename... Args>
const TYPEID *listTypes() {
	static NBIND_CONSTEXPR TYPEID typeList[] = { Typer<Args>::makeID()... };
	return(typeList);
}

} // namespace
