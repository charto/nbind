// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include <emscripten.h>

namespace nbind {

class cbFunction {

public:

	explicit cbFunction(unsigned int num) : num(num) {}

	template<typename... Args>
	void operator()(Args&&... args) {
	}

	template <typename ReturnType, typename... Args>
	ReturnType call(Args... args) {
		// TODO: convert return type!

		return(EM_ASM_INT({return(_nbind.callCallback.apply(this,arguments));}, num, args...));
	}

	unsigned int num;
};

template <> struct BindingType<cbFunction &> {

	typedef cbFunction & Type;

	typedef unsigned int WireType;

//	static inline Type fromWireType(WireType arg);

//	static inline WireType toWireType(Type arg);

};

template<>
struct ArgFromWire<cbFunction &> {

	explicit ArgFromWire(unsigned int num) : val(num) {}

	inline cbFunction &get(unsigned int num) { return(val); }

	cbFunction val;

};

} // namespace
