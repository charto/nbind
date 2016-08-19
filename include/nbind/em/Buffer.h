// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

template<> struct BindingType<Buffer> {

	typedef Buffer Type;

	typedef struct {
		uint32_t length;
		unsigned char *data;
		unsigned int num;
	} *WireType;

	static inline Type fromWireType(WireType arg) {
		return(Buffer(
			arg->data,
			arg->length,
			arg->num
		));
	}

	static inline WireType toWireType(Type arg) { return(nullptr); }

};

inline void Buffer :: commit() {
	EM_ASM_ARGS(
		{_nbind.commitBuffer($0,$1,$2);},
		handle.getNum(), ptr, len
	);
}

} // namespace
