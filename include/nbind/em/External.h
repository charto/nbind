// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

extern "C" {
	extern void _nbind_reference_external(unsigned int num);
	extern void _nbind_free_external(unsigned int num);
}

class External {

public:

	explicit External(unsigned int num) : num(num) {}

	External(const External &other) : num(other.num) {
		_nbind_reference_external(num);
	}

	External (External &&other) : num(other.num) { other.num = 0; }

	External &operator=(const External &other) {
		if(num) _nbind_free_external(num);
		num = other.num;
		_nbind_reference_external(num);

		return(*this);
	}

	External &operator=(External &&other) {
		if(num) _nbind_free_external(num);
		num = other.num;
		other.num = 0;

		return(*this);
	}

	~External() {
		_nbind_free_external(num);
	}

protected:

	unsigned int num;
};

} // namespace
