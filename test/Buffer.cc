// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include <array>
#include <vector>

#include "nbind/api.h"

class Buffer {

public:

	static unsigned int sum(nbind::Buffer buf) {
		size_t length = buf.length();
		unsigned char *data = buf.data();
		unsigned int sum = 0;

		if(!data || !length) return(0);

		for(size_t pos = 0; pos < length; ++pos) {
			sum += data[pos];
		}

		return(sum);
	}

};

#include "nbind/nbind.h"

#ifdef NBIND_CLASS

NBIND_CLASS(Buffer) {
	method(sum);
}

#endif
