// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include "BaseSignature.h"

namespace nbind {

// Constructor. Call() creates an instance of a bound C++ class, while
// createValue() constructs it in place, directly in the stack for passing by value.

template <class Bound, typename... Args>
class ConstructorSignature : public TemplatedBaseSignature<ConstructorSignature<Bound, Args...>, Bound, Args...> {

public:

#if defined(BUILDING_NODE_EXTENSION)
	ConstructorSignature() {
		this->setValueConstructor(reinterpret_cast<funcPtr>(createValue));
	}
#endif // BUILDING_NODE_EXTENSION

	// Unused dummy type.
	typedef void *MethodType;

	typedef TemplatedBaseSignature<ConstructorSignature, Bound, Args...> Parent;

	static constexpr auto typeExpr = BaseSignature::Type::constructor;

#if defined(BUILDING_NODE_EXTENSION)

	typedef Creator<
		Bound,
		typename emscripten::internal::MapWithIndex<
			TypeList,
			ArgFromWire,
			Args...
		>::type
	> ConstructWrapper;

	static void call(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		ConstructWrapper::create(args);
	}

	static void createValue(ArgStorage &storage, const Nan::FunctionCallbackInfo<v8::Value> &args) {
		ConstructWrapper::createValue(storage, args);
	}

#elif defined(EMSCRIPTEN)

	// Args are wire types! They must be received by value.

	static Bound *call(Args... args) {
		return(new Bound(args...));
	}

#endif // BUILDING_NODE_EXTENSION, EMSCRIPTEN

};

} // namespace
