// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include <type_traits>
#include <forward_list>
#include <vector>
#include <stdexcept>

#include "api.h"
#include "wire.h"
#include "MethodDef.h"
#include "ArgStorage.h"

#if defined(BUILDING_NODE_EXTENSION)

#	include "v8/Caller.h"
#	include "signature/BaseSignature.h" // Needs Caller
#	include "v8/Overloader.h" // Needs ArgStorage
#	include "BindClass.h"     // Needs Overloader and BaseSignature
#	include "v8/ValueObj.h"   // Needs BindClass
#	include "v8/Creator.h"    // Needs ArgStorage

#elif defined(EMSCRIPTEN)

#	include "em/Caller.h"
#	include "signature/BaseSignature.h"
#	include "BindClass.h"
#	include "em/ValueObj.h"
#	include "em/Creator.h"    // Needs ArgStorage

#endif

#include "common.h"

#include "signature/FunctionSignature.h"
#include "signature/MethodSignature.h"
#include "signature/GetterSignature.h"
#include "signature/SetterSignature.h"
#include "signature/ConstructorSignature.h"

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

	BindDefiner(const char *name) : BindDefinerBase(name), bindClass(BindClass<Bound>::getInstance()) {
		bindClass.init(name);

		registerClass(bindClass);
	}

	template<class Signature, typename MethodType>
	void addMethod(const char *name, MethodType method) {
		bindClass.addMethod(
			name,
			Signature::getDirect(method),
			Signature::addMethod(method),
			&Signature::getInstance()
		);
	}

	template<template <typename, class, typename, typename...> class Signature, typename ReturnType, typename... Args>
	void addMethodMaybeConst(
		const char* name,
		ReturnType(Bound::*method)(Args...)
	) {
		addMethod<Signature<decltype(method), Bound, ReturnType, Args...>>(name, method);
	}

	template<template <typename, class, typename, typename...> class Signature, typename ReturnType, typename... Args>
	void addMethodMaybeConst(
		const char* name,
		ReturnType(Bound::*method)(Args...) const
	) {
		addMethod<Signature<decltype(method), Bound, ReturnType, Args...>>(name, method);
	}

	template<typename... Args, typename... Policies>
	BindDefiner &constructor(Policies...) {
		typedef ConstructorSignature<Bound, Args...> Signature;

		bindClass.addConstructor(&Signature::getInstance());

		return(*this);
	}

	template<typename ReturnType, typename... Args, typename... Policies>
	BindDefiner &function(
		const char* name,
		ReturnType(*func)(Args...),
		Policies...
	) {
		addMethod<FunctionSignature<decltype(func), Bound, ReturnType, Args...>>(name, func);

		return(*this);
	}

	template<typename MethodType, typename... Policies>
	BindDefiner &function(
		const char* name,
		MethodType method,
		Policies...
	) {
		addMethodMaybeConst<MethodSignature>(name, method);

		return(*this);
	}

	template<typename GetterType, typename... Policies>
	BindDefiner &property(
		const char* name,
		GetterType getter,
		Policies...
	) {
		addMethodMaybeConst<GetterSignature>(name, getter);
		bindClass.addMethod(emptySetter);

		return(*this);
	}

	template<typename GetterType, typename SetterType, typename... Policies>
	BindDefiner &property(
		const char* name,
		GetterType getter,
		SetterType setter,
		Policies...
	) {
		addMethodMaybeConst<GetterSignature>(name, getter);
		addMethodMaybeConst<SetterSignature>(name, setter);

		return(*this);
	}

private:

	BindClass<Bound> &bindClass;

};

} // namespace
