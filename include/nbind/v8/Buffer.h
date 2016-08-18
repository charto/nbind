// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include <node_version.h>

namespace nbind {

template<> struct BindingType<Buffer> {

	typedef Buffer Type;

#if NODE_MODULE_VERSION >= 14 // Node.js 0.12

	static inline Type bufferFromArray(WireType arg) {
		v8::Local<v8::ArrayBuffer> buf;
		unsigned char *data = nullptr;
		size_t offset = 0;
		size_t length;

		if(arg->IsArrayBufferView()) {
			v8::Local<v8::ArrayBufferView> view = arg.template As<v8::ArrayBufferView>();

			buf = view->Buffer();
			offset = view->ByteOffset();
			length = view->ByteLength();
		} else {
			buf = arg.template As<v8::ArrayBuffer>();
			length = buf->ByteLength();
		}

		#if NODE_MODULE_VERSION >= 45 // IO.js 3.0
			data = static_cast<unsigned char *>(buf->GetContents().Data());
		#else
			if(buf->IsExternal()) {
				// If the buffer is already externalized,
				// get the pointer if we have it.
				auto ptr = buf->ToObject()->Get(
					Nan::New<v8::String>("__nbindData").ToLocalChecked()
				);

				if(!ptr->IsExternal()) {
					// Cannot get the data pointer because someone else
					// externalized the buffer. Signal error by returning
					// data of zero length at nullptr.
					offset = 0;
					length = 0;
				} else {
					data = static_cast<unsigned char *>(
						v8::Handle<v8::External>::Cast(ptr)->Value()
					);
				}
			} else {
				// TODO: need to free the data!
				data = static_cast<unsigned char *>(buf->Externalize().Data());

				// Buffer can only be externalized once,
				// so store our precious pointer with it.
				buf->ToObject()->Set(
					Nan::New<v8::String>("__nbindData").ToLocalChecked(),
					Nan::New<v8::External>(data)
				);
			}
		#endif

		return(Buffer(
			data + offset,
			length
		));
	}

#endif

	static inline bool checkType(WireType arg) {
#		if NODE_MODULE_VERSION >= 14 // Node.js 0.12
			if(arg->IsArrayBuffer() || arg->IsArrayBufferView()) return(true);
#		endif

#		if NODE_MODULE_VERSION <= 44 // IO.js 2.0
			if(node::Buffer::HasInstance(arg)) return(true);
#		endif

		return(false);
	}

	static inline Type fromWireType(WireType arg) {
#		if NODE_MODULE_VERSION >= 14 // Node.js 0.12
			if(arg->IsArrayBuffer() || arg->IsArrayBufferView()) {
				return(bufferFromArray(arg));
			}
#		endif

#		if NODE_MODULE_VERSION <= 44 // IO.js 2.0
			if(node::Buffer::HasInstance(arg)) {
				return(Buffer(
					reinterpret_cast<unsigned char *>(node::Buffer::Data(arg)),
					node::Buffer::Length(arg)
				));
			}
#		endif

		return(Buffer(nullptr, 0));
	}

	static inline WireType toWireType(Type arg) {
		return(Nan::Null());
	}

};

} // namespace
