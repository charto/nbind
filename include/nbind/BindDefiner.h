// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include <type_traits>
#include <forward_list>
#include <vector>
#include <stdexcept>

#include "api.h"
#include "wire.h"
#include "Caller.h"
#include "MethodDef.h"
#ifdef BUILDING_NODE_EXTENSION
#include "v8/ConstructorOverload.h"
#include "BindClass.h"
#include "v8/ValueObj.h"
#include "v8/Creator.h"
#include "v8/Binding.h"
#include "v8/ConstructorSignature.h"
#elif EMSCRIPTEN
#include "BindClass.h"
#include "em/Creator.h"
#include "em/Binding.h"
#include "em/TypeID.h"
#endif

#include "signature/FunctionSignature.h"
#include "signature/MethodSignature.h"
#include "signature/GetterSignature.h"
#include "signature/SetterSignature.h"

namespace nbind {

extern const char *emptyGetter;
extern const char *emptySetter;

// BindDefiner is a helper class to make class definition syntax match embind.

class BindDefinerBase {

protected:

	BindDefinerBase(const char *name) : name(name) {}

	const char *name;

};

template <class Bound>
class BindDefiner : public BindDefinerBase {

public:

	BindDefiner(const char *name) : BindDefinerBase(name) {
		bindClass = BindClass<Bound>::getInstance();
		bindClass->setName(name);

		Bindings::registerClass(bindClass);
	}

	template<class Signature, typename MethodType>
	void addMethod(const char *name, MethodType method) const {
		auto &def = bindClass->addMethod(
			name,
			Signature::addMethod(method),
			&Signature::getInstance()
		);

#ifdef EMSCRIPTEN
		def.emInit(Signature::getEmSignature());
#else
		(void)def;
#endif
	}

	template<typename ReturnType, typename... Args, typename... Policies>
	const BindDefiner &function(
		const char* name,
		ReturnType(Bound::*method)(Args...),
		Policies...
	) const {
		addMethod<MethodSignature<Bound, ReturnType, Args...>>(name, method);

		return(*this);
	}

	template<typename ReturnType, typename... Args, typename... Policies>
	const BindDefiner &function(
		const char* name,
		ReturnType(*func)(Args...),
		Policies...
	) const {
		addMethod<FunctionSignature<ReturnType, Args...>>(name, func);

		return(*this);
	}

	template<typename... Args, typename... Policies>
	const BindDefiner &constructor(Policies...) const {
#ifdef BUILDING_NODE_EXTENSION
		typedef typename ConstructorSignature<Bound, Args...>::ConstructWrapper Constructor;

		ConstructorOverload<Bound>::addConstructor(sizeof...(Args), Constructor::makeWrapper, Constructor::makeValue);
#endif // BUILDING_NODE_EXTENSION

		return(*this);
	}

	template<
		typename FieldType,
		typename... Policies
	>
	const BindDefiner &property(
		const char* name,
		FieldType(Bound::*getter)(),
		Policies...
	) const {
		addMethod<GetterSignature<Bound, FieldType>>(name, getter);

		bindClass->addMethod(emptySetter);

		return(*this);
	}

	template<
		typename GetFieldType,
		typename SetFieldType,
		typename SetReturnType,
		typename... Policies
	>
	const BindDefiner &property(
		const char* name,
		GetFieldType(Bound::*getter)(),
		SetReturnType(Bound::*setter)(SetFieldType),
		Policies...
	) const {
		addMethod<GetterSignature<Bound, GetFieldType>>(name, getter);

		addMethod<SetterSignature<Bound, SetReturnType, SetFieldType>>(name,setter);

		return(*this);
	}

private:

	BindClass<Bound> *bindClass;

};

} // namespace
