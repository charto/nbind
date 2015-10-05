// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

template<class Bound, typename... Args>
struct Creator {
	static inline Bound *create(typename BindingType<Args>::WireType... args) {
		return(new Bound(ArgFromWire<Args>(args).get(args)...));
	}

	static void createValue(ArgStorage &storage, typename BindingType<Args>::WireType... args) {
		static_cast<TemplatedArgStorage<Bound> &>(storage).init(ArgFromWire<Args>(args).get(args)...);
	}

};

} // namespace
