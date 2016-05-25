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

template <typename ArgType, size_t size>
struct BindingType<std::array<ArgType, size>> {

	typedef std::array<ArgType, size> type;

	static inline bool checkType(WireType arg) {
		if(!arg->IsArray()) return(false);

		v8::Local<v8::Array> arr = arg.template As<v8::Array>();

		return(arr->Length() >= size);
	}

	static inline type fromWireType(WireType arg) {
		// TODO: Don't convert sparse arrays.

		v8::Local<v8::Array> arr = arg.template As<v8::Array>();

		type val;

		// Length of arr is checked in checkType().
		for(uint32_t num = 0; num < size; ++num) {
			v8::Local<v8::Value> item;

			if(
				Nan::Get(arr, num).ToLocal(&item) &&
				BindingType<ArgType>::checkType(item)
			) {
				val[num] = BindingType<ArgType>::fromWireType(item);
			} else {
				throw(std::runtime_error("Error converting array element"));
			}
		}

		return(val);
	}

	static inline WireType toWireType(type arg) {
		v8::Local<v8::Array> arr = Nan::New<v8::Array>(size);

		for(uint32_t num = 0; num < size; ++num) {
			arr->Set(num, BindingType<ArgType>::toWireType(arg[num]));
		}

		return(arr);
	}

};

// Vector.

template <typename ArgType>
struct BindingType<std::vector<ArgType>> {

	typedef std::vector<ArgType> type;

	static inline bool checkType(WireType arg) {
		return(arg->IsArray());
	}

	static inline type fromWireType(WireType arg) {
		// TODO: Don't convert sparse arrays.

		v8::Local<v8::Array> arr = arg.template As<v8::Array>();
		uint32_t count = arr->Length();

		// We know the length, so it's faster to preallocate the vector.

		type val;
		val.reserve(count);

		for(uint32_t num = 0; num < count; ++num) {
			v8::Local<v8::Value> item;

			if(
				Nan::Get(arr, num).ToLocal(&item) &&
				BindingType<ArgType>::checkType(item)
			) {
				val.push_back(BindingType<ArgType>::fromWireType(item));
			} else {
				throw(std::runtime_error("Error converting array element"));
			}
		}

		return(val);
	}

	static inline WireType toWireType(type arg) {
		uint32_t count = arg.size();
		v8::Local<v8::Array> arr = Nan::New<v8::Array>(count);

		for(uint32_t num = 0; num < count; ++num) {
			arr->Set(num, BindingType<ArgType>::toWireType(arg[num]));
		}

		return(arr);
	}

};

// String.

template <> struct BindingType<std::string> {

	typedef std::string type;

	static inline bool checkType(WireType arg) {
		return(arg->IsString());
	}

	static inline type fromWireType(WireType arg) {
		Nan::Utf8String val(arg->ToString());
		return(std::string(*val, val.length()));
	}

	static inline WireType toWireType(type arg) {
		return(Nan::New<v8::String>(arg.c_str(), arg.length()).ToLocalChecked());
	}

};

} // namespace
