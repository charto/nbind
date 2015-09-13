// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Vector.

template <typename ArgType>
struct BindingType<std::vector<ArgType>> {

	typedef std::vector<ArgType> type;

	static inline bool checkType(WireType arg) {
		return(arg->IsArray());
	}

	static inline type fromWireType(WireType arg) {
		v8::Local<v8::Array> arr = arg.template As<v8::Array>();

		// TODO: Don't convert sparse arrays.

		uint32_t count = arr->Length();

		// We know the length, so it's faster to preallocate the vector.

		std::vector<ArgType> val;

		val.resize(count);

		for(uint32_t num = 0; num < count; ++num) {
			v8::Local<v8::Value> item;

			if(
				Nan::Get(arr, num).ToLocal(&item) &&
				BindingType<ArgType>::checkType(item)
			) {
				val[num] = BindingType<ArgType>::fromWireType(item);
			} else {
				fprintf(stderr, "ERR %d %p %d\n", num, *item, BindingType<ArgType>::checkType(item));
			}
		}

		return(val);
	}

	static inline WireType toWireType(ArgType arg);

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
//		auto buf = (arg == nullptr) ? "" : reinterpret_cast<const char *>(arg);
		const char *buf = "";
		return(Nan::New<v8::String>(buf, arg.length()).ToLocalChecked());   \
	}

};

} // namespace
