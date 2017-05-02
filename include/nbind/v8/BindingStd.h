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

	typedef std::array<ArgType, size> Type;

	static inline bool checkType(WireType arg) {
		if(!arg->IsArray()) return(false);

		v8::Local<v8::Array> arr = arg.template As<v8::Array>();

		return(arr->Length() >= size);
	}

	static inline Type fromWireType(WireType arg) {
		// TODO: Don't convert sparse arrays.

		v8::Local<v8::Array> arr = arg.template As<v8::Array>();

		Type val;

		// Length of arr is checked in checkType().
		for(uint32_t num = 0; num < size; ++num) {
			v8::Local<v8::Value> item;

			if(
				Nan::Get(arr, num).ToLocal(&item) &&
				BindingType<ArgType>::checkType(item)
			) {
				val[num] = convertFromWire<ArgType>(item);
			} else {
				throw(std::runtime_error("Error converting array element"));
			}
		}

		return(val);
	}

	static inline WireType toWireType(Type &&arg) {
		v8::Local<v8::Array> arr = Nan::New<v8::Array>(size);

		for(uint32_t num = 0; num < size; ++num) {
			arr->Set(num, convertToWire(std::forward<ArgType>(arg[num])));
		}

		return(arr);
	}

};

// Vector.

template <typename ArgType>
struct BindingType<std::vector<ArgType>> {

	typedef std::vector<ArgType> Type;

	static inline bool checkType(WireType arg) {
		return(arg->IsArray());
	}

	static inline Type fromWireType(WireType arg) {
		// TODO: Don't convert sparse arrays.

		v8::Local<v8::Array> arr = arg.template As<v8::Array>();
		uint32_t count = arr->Length();

		// We know the length, so it's faster to preallocate the vector.

		Type val;
		val.reserve(count);

		for(uint32_t num = 0; num < count; ++num) {
			v8::Local<v8::Value> item;

			if(
				Nan::Get(arr, num).ToLocal(&item) &&
				BindingType<ArgType>::checkType(item)
			) {
				val.push_back(convertFromWire<ArgType>(item));
			} else {
				throw(std::runtime_error("Error converting array element"));
			}
		}

		return(val);
	}

	static inline WireType toWireType(Type &&arg) {
		uint32_t count = arg.size();
		v8::Local<v8::Array> arr = Nan::New<v8::Array>(count);

		for(uint32_t num = 0; num < count; ++num) {
			arr->Set(num, convertToWire(std::forward<ArgType>(arg[num])));
		}

		return(arr);
	}

};

// String.

template <> struct BindingType<std::string> {

	typedef std::string Type;

	static inline bool checkType(WireType arg) {
		return(true);
	}

	static inline Type fromWireType(WireType arg) {
		Nan::Utf8String val(arg->ToString());
		return(std::string(*val, val.length()));
	}

	static inline WireType toWireType(Type arg) {
		return(Nan::New<v8::String>(arg.c_str(), arg.length()).ToLocalChecked());
	}

};

template <> struct BindingType<StrictType<std::string>> : public BindingType<std::string> {
	static inline bool checkType(WireType arg) {
		return(arg->IsString());
	}
};

// String reference.

template <> struct BindingType<const std::string &> {

	typedef std::string Type;

	static inline bool checkType(WireType arg) {
		return(true);
	}

};

template <> struct BindingType<StrictType<const std::string &>> : public BindingType<const std::string &> {
	static inline bool checkType(WireType arg) {
		return(arg->IsString());
	}
};

template<typename PolicyList, size_t Index>
struct ArgFromWire<PolicyList, Index, const std::string &> {

	// TODO: Get string length from JS to support zero bytes?
	template <typename NanArgs>
	ArgFromWire(const NanArgs &args) : val(*Nan::Utf8String(args[Index]->ToString())) {}

	template <typename NanArgs>
	inline const std::string &get(const NanArgs &args) {
		return(val);
	}

	// RAII style storage for the string data.

	std::string val;

};

} // namespace
