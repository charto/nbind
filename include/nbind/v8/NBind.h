// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

class cbFunction;

class NBind {

public:

	static void bind_value(const char *name, cbFunction &func);

	static void reflect(
		cbFunction &outClass,
		cbFunction &outMethod
	);

};

} // namespace

#define nbind_debug()
