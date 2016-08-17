// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

class Buffer {

public:

	Buffer(unsigned char *ptr, size_t len);

	unsigned char *data();
	size_t length();

private:

	unsigned char *ptr;
	size_t len;

};

} // namespace
