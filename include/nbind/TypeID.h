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
static constexpr TYPEID makeTypeID() {
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

template<typename... Args>
TYPEID *listTypes() {
	static constexpr TYPEID typeList[] = { TypeID<Args>::get()... };
	return(typeList);
}

} // namespace
