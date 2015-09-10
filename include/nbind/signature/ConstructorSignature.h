// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include "BaseSignature.h"

namespace nbind {

template <class Bound, typename... Args>
class ConstructorSignature : public TemplatedBaseSignature<ConstructorSignature<Bound, Args...>, Bound, Args...> {

public:

#ifdef BUILDING_NODE_EXTENSION
	ConstructorSignature() {
		this->setValueConstructor(reinterpret_cast<funcPtr>(createValue));
	}
#endif // BUILDING_NODE_EXTENSION

	// Unused dummy type.
	typedef void *MethodType;

	typedef TemplatedBaseSignature<ConstructorSignature, Bound, Args...> Parent;

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
		ConstructWrapper::create(args);
	}

	static void createValue(ArgStorage &storage, const Nan::FunctionCallbackInfo<v8::Value> &args) {
		ConstructWrapper::createValue(storage, args);
	}
#else
	static void call() {}
#endif // BUILDING_NODE_EXTENSION

};

} // namespace
