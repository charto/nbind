// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

template<typename PolicyList, typename ArgType>
struct ArgFromWire {

	typedef TypeTransformer<ArgType, PolicyList> Transformed;
	typedef typename Transformed::WireType WireType;

	explicit ArgFromWire(WireType arg) {}

	inline typename Transformed::Type get(WireType arg) const {
		return(Transformed::Binding::fromWireType(arg));
	}

};

template<typename PolicyList>
struct ArgFromWire<PolicyList, void> {

	explicit ArgFromWire() {}

	inline void get() const {}

};

} // namespace
