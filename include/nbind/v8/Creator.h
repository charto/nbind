// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

template<class Bound, typename ArgList> struct Creator;

template<class Bound, typename... Args>
struct Creator<Bound, TypeList<Args...>> {

public:

	static void create(const Nan::FunctionCallbackInfo<v8::Value> &args) noexcept(false) {
		// Note that Args().get may throw.
		BindWrapper<Bound>::createObj(args, Args(args).get(args)...);
	}

	static void createValue(ArgStorage &storage, const Nan::FunctionCallbackInfo<v8::Value> &args) {
		static_cast<TemplatedArgStorage<Bound> &>(storage).init(Args(args).get(args)...);
	}

};

} // namespace
