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
#include "BindClass.h"
#ifdef BUILDING_NODE_EXTENSION
#include "v8/ValueObj.h"
#include "v8/Binding.h"
#elif EMSCRIPTEN
#include "em/Binding.h"
#endif

#include "FunctionSignature.h"
#include "MethodSignature.h"
#include "AccessorSignature.h"

namespace nbind {

// BindDefiner is a helper class to make class definition syntax match embind.

class BindDefinerBase {

protected:

	BindDefinerBase(const char *name) : name(strdup(name)) {}

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

	template<typename ReturnType, typename... Args, typename... Policies>
	const BindDefiner &function(
		const char* name,
		ReturnType(Bound::*method)(Args...),
		Policies...
	) const {
		typedef MethodSignature<Bound, ReturnType, Args...> Signature;

		bindClass->addMethod(
			BindClassBase::MethodDef::Type::method,
			name,
			Signature::addMethod(method),
			reinterpret_cast<funcPtr>(Signature::call)
		);

		return(*this);
	}

	template<typename ReturnType, typename... Args, typename... Policies>
	const BindDefiner &function(
		const char* name,
		ReturnType(*func)(Args...),
		Policies...
	) const {
		typedef FunctionSignature<ReturnType, Args...> Signature;

		bindClass->addMethod(
			BindClassBase::MethodDef::Type::function,
			name,
			Signature::addMethod(func),
			reinterpret_cast<funcPtr>(Signature::call)
		);

		return(*this);
	}

	template<typename... Args, typename... Policies>
	const BindDefiner &constructor(Policies...) const {
#ifdef BUILDING_NODE_EXTENSION
		typedef ConstructorInfo<
			Bound,
			typename emscripten::internal::MapWithIndex<
				TypeList,
				FromWire,
				Args...
			>::type
		> Constructor;

		Constructor::setClassName(this->name);
		bindClass->addConstructor(sizeof...(Args), Constructor::makeWrapper, Constructor::makeValue);
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
		typedef AccessorSignature<Bound, FieldType> GetterSignature;

		bindClass->addMethod(
			BindClassBase::MethodDef::Type::getter,
			name,
			GetterSignature::addMethod(getter),
			reinterpret_cast<funcPtr>(GetterSignature::getter)
		);

		bindClass->addMethod(
			BindClassBase::MethodDef::Type::setter,
			name,
			0,
			nullptr
		);

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
		typedef AccessorSignature<Bound, GetFieldType> GetterSignature;
		typedef AccessorSignature<Bound, SetReturnType, SetFieldType> SetterSignature;

		bindClass->addMethod(
			BindClassBase::MethodDef::Type::getter,
			name,
			GetterSignature::addMethod(getter),
			reinterpret_cast<funcPtr>(GetterSignature::getter)
		);

		bindClass->addMethod(
			BindClassBase::MethodDef::Type::setter,
			name,
			SetterSignature::addMethod(setter),
			reinterpret_cast<funcPtr>(SetterSignature::setter)
		);

		return(*this);
	}

private:

	BindClass<Bound> *bindClass;

};

} // namespace
