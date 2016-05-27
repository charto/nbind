// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

class NBind {

public:

	static void bind_value(const char *name, cbFunction &func);
	static void *lalloc(size_t size);
	static void lreset(unsigned int used, unsigned char *page);

};

} // namespace
