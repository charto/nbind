// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

template<typename PolicyList, class Bound, typename... Args>
struct Creator {

	typedef typename BindingType<Bound>::WireType WireType;

	static inline WireType create(typename BindingType<Args>::WireType... args) {
		// return(new Bound(ArgFromWire<PolicyList, Args>(args).get(args)...));

		WireType val = reinterpret_cast<WireType>(NBind::lalloc(sizeof(*val)));

		val->boundUnsafe = new Bound(ArgFromWire<PolicyList, Args>(args).get(args)...);
		val->boundShared = new std::shared_ptr<Bound>(val->boundUnsafe);

		return(val);
	}

	static void createValue(ArgStorage &storage, typename BindingType<Args>::WireType... args) {
		static_cast<TemplatedArgStorage<Bound> &>(storage).init(ArgFromWire<PolicyList, Args>(args).get(args)...);
	}

};

} // namespace
