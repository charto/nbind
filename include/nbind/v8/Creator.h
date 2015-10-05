// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

template<class Bound, typename ArgList> struct Creator;

template<class Bound, typename... Args>
struct Creator<Bound, TypeList<Args...>> {

public:

	static void create(const Nan::FunctionCallbackInfo<v8::Value> &args) noexcept(false) {
		if(args[0]->IsExternal()) {
			Bound *ptr = static_cast<Bound *>(v8::Handle<v8::External>::Cast(args[0])->Value());

			BindWrapper<Bound>::createPtr(ptr)->wrapThis(args);
		} else {
			// Note that Args().get may throw.

			BindWrapper<Bound>::createObj(Args(args).get(args)...)->wrapThis(args);
		}
	}

	static void createValue(ArgStorage &storage, const Nan::FunctionCallbackInfo<v8::Value> &args) {
		static_cast<TemplatedArgStorage<Bound> &>(storage).init(Args(args).get(args)...);
	}

};

} // namespace
