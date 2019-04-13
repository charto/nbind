// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// ArgFromWire converts JavaScript types into C++ types, usually with BindingType<>::fromWireType
// but some types require additional temporary storage, such as a string converted to C style.
// FromWire is a struct, so wrappers for all objects can be constructed as function arguments,
// and their actual values passed to the called function are returned by the get() function.
// The wrappers go out of scope and are destroyed at the end of the function call.

// Handle most C++ types.

template<typename PolicyList, size_t Index, typename ArgType>
struct ArgFromWire {

	typedef TypeTransformer<ArgType, PolicyList> Transformed;

	template <typename NanArgs>
	ArgFromWire(const NanArgs &args) {}

	template <typename NanArgs>
	inline typename Transformed::Type get(const NanArgs &args) noexcept(false) {
		return(Transformed::Binding::fromWireType(args[Index]));
	}

};

// Handle char pointers, which will receive a C string representation of any JavaScript value.

template<typename PolicyList, size_t Index>
struct ArgFromWire<PolicyList, Index, const char *> {

	template <typename NanArgs>
	ArgFromWire(const NanArgs &args) : val(Nan::To<v8::String>(args[Index]).ToLocalChecked()) {}

	template <typename NanArgs>
	inline const char *get(const NanArgs &args) {
		return(*val);
	}

	// RAII style storage for the string data.

	Nan::Utf8String val;

};

// Automatically cast char to unsigned if the C++ function expects it.

template<typename PolicyList, size_t Index>
struct ArgFromWire<PolicyList, Index, const unsigned char *> {

	template <typename NanArgs>
	ArgFromWire(const NanArgs &args) : val(Nan::To<v8::String>(args[Index]).ToLocalChecked()) {}

	template <typename NanArgs>
	inline const unsigned char *get(const NanArgs &args) {
		return(reinterpret_cast<const unsigned char *>(*val));
	}

	// RAII style storage for the string data.

	Nan::Utf8String val;

};

} // namespace
