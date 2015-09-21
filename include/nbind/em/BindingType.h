// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// Convert between JavaScript types used in the V8 engine and native C++ types.
// Following emscripten conventions, the type passed between the two is called
// WireType.

#pragma once

namespace nbind {

template <typename ArgType> struct BindingType {

	typedef ArgType Type;

	typedef ArgType WireType;

//	static inline Type fromWireType(WireType arg);

//	static inline WireType toWireType(Type arg);

};

template<typename ArgType>
struct ArgFromWire {

	explicit ArgFromWire(ArgType arg) {}

	inline ArgType get(ArgType arg) { return(arg); }

};

} // namespace
