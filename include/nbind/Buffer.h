// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

class Buffer {

	friend struct BindingType<Buffer>;

public:

#if defined(BUILDING_NODE_EXTENSION)

	Buffer(unsigned char *ptr = nullptr, size_t len = 0) :
		ptr(ptr), len(len) {}

	Buffer(unsigned char *ptr, size_t len, v8::Local<v8::Object> obj) :
		ptr(ptr), len(len), handle(obj) {}

#elif defined(EMSCRIPTEN)

	Buffer(unsigned char *ptr = nullptr, size_t len = 0, unsigned int num = 0) :
		ptr(ptr), len(len), handle(num) {}

#endif

	inline unsigned char *data() const { return(ptr); }

	inline size_t length() const { return(len); }

	inline void commit();

private:

	unsigned char *ptr;
	size_t len;
	External handle;

};

} // namespace
