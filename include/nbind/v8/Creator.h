// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

template<class Bound, typename ArgList> struct Creator;

template<class Bound, typename... Args>
struct Creator<Bound, TypeList<Args...>> {

public:

	// Make sure prototype matches NanWrapperConstructorTypeBuilder!
	static BindWrapper<Bound> *create(const Nan::FunctionCallbackInfo<v8::Value> &args) noexcept(false) {
		// Note that Args().get may throw.
		return(new BindWrapper<Bound>(Args(args).get(args)...));
	}

	// Make sure prototype matches NanWrapperConstructorTypeBuilder!
	static void create2(const Nan::FunctionCallbackInfo<v8::Value> &args) noexcept(false) {
		// Note that Args().get may throw.
		(new BindWrapper<Bound>(Args(args).get(args)...))->wrap(args);
	}

	// Make sure prototype matches NanValueConstructorTypeBuilder!
	static void createValue(ArgStorage<Bound> &storage, const Nan::FunctionCallbackInfo<v8::Value> &args) {
		storage.init(Args(args).get(args)...);
	}

};

} // namespace
