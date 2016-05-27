// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

class Pool {

public:

	// TODO: testbench should try values 8 and 65536 here.
	static const unsigned int pageSize = 65536;
	static unsigned int used;
	static unsigned char *rootPage;
	static unsigned char *page;

};

class PoolRestore {

public:

	PoolRestore();
	~PoolRestore();

private:

	unsigned int used;
	unsigned char *page;

};

} // namespace
