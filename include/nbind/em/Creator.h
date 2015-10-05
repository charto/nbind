// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

template<class Bound, typename... Args>
struct Creator {
	static inline Bound *create(Args... args) {
		return(new Bound(args...));
	}

	static void createValue(ArgStorage &storage, Args... args) {
		static_cast<TemplatedArgStorage<Bound> &>(storage).init(args...);
	}

};

} // namespace
