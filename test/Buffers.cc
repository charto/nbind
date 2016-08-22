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

	static void mul2(nbind::Buffer buf) {
		size_t length = buf.length();
		unsigned char *data = buf.data();

		if(!data || !length) return;

		for(size_t pos = 0; pos < length; ++pos) {
			data[pos] *= 2;
		}

		buf.commit();
	}

};

#include "nbind/nbind.h"

#ifdef NBIND_CLASS

NBIND_CLASS(Buffer) {
	method(sum);
	method(mul2);
}

#endif
