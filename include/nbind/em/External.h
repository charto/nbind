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

	explicit External(unsigned int num = 0) : num(num) {}

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
		if(num) _nbind_free_external(num);
	}

	inline unsigned int getNum() const { return(num); }

private:

	unsigned int num;
};

template<>
struct BindingType<External> {

	typedef External Type;
	typedef int WireType;

	static inline Type fromWireType(WireType arg) { return(External(arg)); }
	static inline WireType toWireType(Type arg) { return(arg.getNum()); }

};

} // namespace
