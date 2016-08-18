// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

class Buffer : public External {

public:

#if defined(BUILDING_NODE_EXTENSION)
	Buffer(unsigned char *ptr, size_t len) :
		ptr(ptr), len(len) {}
#elif defined(EMSCRIPTEN)
	Buffer(unsigned char *ptr, size_t len, unsigned int num = 0) :
		External(num), ptr(ptr), len(len) {}
#endif

	inline unsigned char *data() const { return(ptr); }

	inline size_t length() const { return(len); }

	inline void commit();

private:

	unsigned char * const ptr;
	const size_t len;

};

} // namespace
