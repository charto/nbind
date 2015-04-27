// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include "wire.h"
#include "FunctionSignature.h"
#include "MethodSignature.h"
#include "AccessorSignature.h"

namespace nbind {

template<class Bound, typename ArgList> struct ConstructorInfo;

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

		Signature::setClassName(this->name);
		bindClass->addMethod(
			name,
			Signature::addFunction(name, method),
			Signature::call
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

		bindClass->addFunction(
			name,
			Signature::addFunction(name, func),
			Signature::call
		);

		return(*this);
	}

	template<typename... Args, typename... Policies>
	const BindDefiner &constructor(Policies...) const {
		typedef ConstructorInfo<
			Bound,
			typename emscripten::internal::MapWithIndex<
				TypeList,
				FromWire,
				Args...
			>::type
		> Constructor;

		Constructor::setClassName(this->name);
		bindClass->addConstructor(sizeof...(Args), Constructor::call);

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

		GetterSignature::setClassName(this->name);

		bindClass->addAccessor(
			name,
			GetterSignature::addFunction(name, getter),
			0,
			GetterSignature::getter,
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

		GetterSignature::setClassName(this->name);
		SetterSignature::setClassName(this->name);

		bindClass->addAccessor(
			name,
			GetterSignature::addFunction(name, getter),
			SetterSignature::addFunction(name, setter),
			GetterSignature::getter,
			SetterSignature::setter
		);

		return(*this);
	}

private:

	BindClass<Bound> *bindClass;

};

} // namespace
