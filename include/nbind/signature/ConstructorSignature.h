// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include "BaseSignature.h"

namespace nbind {

// Constructor. Call() creates an instance of a bound C++ class, while
// createValue() constructs it in place, directly in the stack for passing by value.

template <class Bound, typename PolicyList, typename... Args>
class ConstructorSignature : public TemplatedBaseSignature<ConstructorSignature<Bound, PolicyList, Args...>, PolicyList, Bound *, Args...> {

public:

	ConstructorSignature() {
		this->setValueConstructor(reinterpret_cast<funcPtr>(createValue));
	}

	// Unused dummy type.
	typedef void *MethodType;

	typedef TemplatedBaseSignature<ConstructorSignature, PolicyList, Bound *, Args...> Parent;

	static constexpr auto typeExpr = BaseSignature :: SignatureType :: construct;

#if defined(BUILDING_NODE_EXTENSION)

	typedef Creator<
		Bound,
		typename emscripten::internal::MapWithIndex<
			PolicyList,
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

#elif defined(__EMSCRIPTEN__)

	typedef Creator<
		PolicyList,
		Bound,
		Args...
	> ConstructWrapper;

	// Args are wire types! They must be received by value.

	static typename ConstructWrapper::WireType call(
		typename BindingType<Args>::WireType... args
	) {
		return(ConstructWrapper::create(args...));
	}

	static void createValue(
		ArgStorage &storage,
		typename BindingType<Args>::WireType... args
	) {
		ConstructWrapper::createValue(storage, args...);
	}

#endif // BUILDING_NODE_EXTENSION, __EMSCRIPTEN__

};

} // namespace
