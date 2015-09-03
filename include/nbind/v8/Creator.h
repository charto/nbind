// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

template<class Bound, typename ArgList> struct Creator;

template<class Bound, typename... Args>
struct Creator<Bound, TypeList<Args...>> {

public:

	// Make sure prototype matches NanWrapperConstructorTypeBuilder!
	template <typename... NanArgs>
	static BindWrapper<Bound> *makeWrapper(NanArgs... args) noexcept(false) {
		// Note that Args().get may throw.
		return(new BindWrapper<Bound>(Args(std::forward<NanArgs>(args)...).get(args...)...));
	}

	// Make sure prototype matches NanValueConstructorTypeBuilder!
	template <typename... NanArgs>
	static void makeValue(ArgStorage<Bound> &storage, NanArgs... args) {
		storage.init(Args(std::forward<NanArgs>(args)...).get(args...)...);
	}

};

} // namespace
