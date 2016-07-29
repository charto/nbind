// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

class cbFunction;

class NBind {

public:

	static void bind_value(const char *name, cbFunction &func);

	static void reflect(
		cbFunction &outPrimitive,
		cbFunction &outType,
		cbFunction &outClass,
		cbFunction &outMethod
	);

	static void queryType(uintptr_t type, cbFunction &outTypeDetail);

	static uintptr_t lalloc(size_t size);
	static void lreset(unsigned int used, uintptr_t page);

};

} // namespace

extern "C" {
    extern void nbind_debug(void);
}
