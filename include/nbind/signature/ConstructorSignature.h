// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include "BaseSignature.h"

namespace nbind {

template <class Bound, typename... Args>
class ConstructorSignature2 : public TemplatedBaseSignature<ConstructorSignature2<Bound, Args...>, Bound, Args...> {

public:

	// Unused dummy type.
	typedef void *MethodType;

	typedef TemplatedBaseSignature<ConstructorSignature2, Bound, Args...> Parent;

	static constexpr auto typeExpr = BaseSignature::Type::constructor;

#ifdef BUILDING_NODE_EXTENSION
	typedef Creator<
		Bound,
		typename emscripten::internal::MapWithIndex<
			TypeList,
			FromWire,
			Args...
		>::type
	> ConstructWrapper;

	static void call(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		Status::clearError();

		// Call C++ constructor and bind the resulting object
		// to the new JavaScript object being created.

		try {
			ConstructWrapper::create(args);

			const char *message = Status::getError();

			if(message) {
				Nan::ThrowError(message);
				return;
			}

			args.GetReturnValue().Set(args.This());
		} catch(const std::exception &ex) {
			const char *message = Status::getError();

			if(message == nullptr) message = ex.what();

			Nan::ThrowError(message);
		}
	}
#else
	static void call() {}
#endif // BUILDING_NODE_EXTENSION

};

} // namespace
