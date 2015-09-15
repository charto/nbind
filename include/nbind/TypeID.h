// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Type ID system, based on Embind.

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

template <typename ArgType>
static NBIND_CONSTEXPR TYPEID makeTypeID() {
	return(TypeStore<ArgType>::get());
}

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
void **defineTypes() {
	static TYPEID typeList[] = { makeTypeID<Args>()..., nullptr };
	static uint32_t sizeList[] = { sizeof(typename std::remove_pointer<Args>::type)... };
	static uint8_t flagList[] = { (
		isChar<typename std::remove_pointer<Args>::type>::value * 16 |
		std::is_const<typename std::remove_pointer<Args>::type>::value * 8 |
		std::is_pointer<Args>::value * 4 |
		// Type is floating point?
		((typename std::remove_pointer<Args>::type)1/2 != 0) * 2 |
		// Type is unsigned?
		((typename std::remove_pointer<Args>::type)-1 >= 0)
	)... };

	static void *data[] = { reinterpret_cast<void *>(sizeof...(Args)), typeList, sizeList, flagList };
	return(data);
}

// Convert a list of types in the template argument into an array of type IDs.

template<typename... Args>
const TYPEID *listTypes() {
	static NBIND_CONSTEXPR TYPEID typeList[] = { makeTypeID<Args>()... };
	return(typeList);
}

} // namespace
