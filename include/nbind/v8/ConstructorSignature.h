// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

template <class Bound, typename... Args>
class ConstructorSignature {

public:

	typedef Creator<
		Bound,
		typename emscripten::internal::MapWithIndex<
			TypeList,
			FromWire,
			Args...
		>::type
	> ConstructWrapper;

};

} // namespace
