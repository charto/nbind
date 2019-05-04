// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include <node_version.h>

namespace nbind {

template<> struct BindingType<Buffer> {

	typedef Buffer Type;

#if NODE_MODULE_VERSION >= 14 // >= Node.js 0.12

	static void initFromArray(WireType arg, Buffer &result);

#endif // >= Node.js 0.12

	static inline bool checkType(WireType arg) {
#		if NODE_MODULE_VERSION >= 14 // Node.js 0.12
			if(arg->IsArrayBuffer() || arg->IsArrayBufferView()) return(true);
#		endif

#		if NODE_MODULE_VERSION < 45 // IO.js 3.0
			if(node::Buffer::HasInstance(arg)) return(true);
#		endif

		return(false);
	}

	static inline Type fromWireType(WireType arg) {
		Buffer result(nullptr, 0, Nan::To<v8::Object>(arg).ToLocalChecked());

#		if NODE_MODULE_VERSION >= 14 // Node.js 0.12
			if(arg->IsArrayBuffer() || arg->IsArrayBufferView()) {
				initFromArray(arg, result);
			}
#		endif

#		if NODE_MODULE_VERSION < 45 // IO.js 3.0
			if(node::Buffer::HasInstance(arg)) {
				result.ptr = reinterpret_cast<unsigned char *>(node::Buffer::Data(arg));
				result.len = node::Buffer::Length(arg);
			}
#		endif

		return(result);
	}

	static inline WireType toWireType(Type arg) {
		return(Nan::Null());
	}

};

inline void Buffer :: commit() {}

} // namespace
