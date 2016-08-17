// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

template<>
struct BindingType<Buffer> {

	typedef Buffer Type;

	static inline bool checkType(WireType arg) {
		return(true);
	}

	static inline Type fromWireType(WireType arg) {
		// v8::Uint8ClampedArray
		v8::Local<v8::ArrayBufferView> view = arg.template As<v8::ArrayBufferView>();
		v8::Local<v8::ArrayBuffer> buf = view->Buffer();

		fprintf(stderr, "FLAGS: %d %d\n", buf->IsExternal(), buf->IsNeuterable());

		return(Buffer(
			static_cast<unsigned char *>(buf->GetContents().Data()),
			view->ByteLength()
		));
	}

	static inline WireType toWireType(Type arg) {
		return(Nan::Null());
	}

};

} // namespace
