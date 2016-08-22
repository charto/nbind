// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#ifdef BUILDING_NODE_EXTENSION

#include "nbind/nbind.h"

using namespace nbind;

#if NODE_MODULE_VERSION >= 14 // >= Node.js 0.12

void BindingType<Buffer> :: initFromArray(WireType arg, Buffer &result) {
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

#	if NODE_MODULE_VERSION >= 45 // >= IO.js 3.0
		data = static_cast<unsigned char *>(buf->GetContents().Data());
#	else // < IO.js 3.0
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
			data = static_cast<unsigned char *>(buf->Externalize().Data());

			// Set callback to free the data.
			result.handle.addDestructor(&Buffer :: destroy, data);

			// Buffer can only be externalized once,
			// so store our precious pointer with it.
			buf->ToObject()->Set(
				Nan::New<v8::String>("__nbindData").ToLocalChecked(),
				Nan::New<v8::External>(data)
			);
		}
#	endif // < IO.js 3.0

	result.ptr = data + offset;
	result.len = length;
}

#endif // >= Node.js 0.12

#endif
