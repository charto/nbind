// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
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

// BindDefiner is a helper class to allow making
// class definition syntax match embind.

template <class Bound>
class BindDefiner {

public:

	BindDefiner(const char *name) : bindClass(BindClass<Bound>::getInstance()) {
		bindClass.init(name);

#		if defined(BUILDING_NODE_EXTENSION)
			// Set up handler to wrap object pointers instantiated in C++
			// for use in JavaScript.

			bindClass.wrapPtr = BindWrapper<Bound>::wrapPtr;
#		endif

		registerClass(bindClass);
	}

	template <
		class Signature,
		typename MethodType
	> void addMethod(
		const char *name,
		MethodType method
	) {
		bindClass.addMethod(
			name,
			Signature::getDirect(method),
			Signature::addMethod(method),
			&Signature::getInstance()
		);
	}

	template <
		template <typename, class, typename, typename...> class Signature,
		typename ReturnType,
		typename... Args,
		typename... Policies
	> void addMethodMaybeConst(
		const char* name,
		ReturnType(Bound::*method)(Args...),
		Policies... policies
	) {
		addMethod<
			Signature<decltype(method), Bound, PolicyListType<Policies...>, ReturnType, Args...>,
			decltype(method)
		>(name, method);
	}

	template <
		template <typename, class, typename, typename...> class Signature,
		typename ReturnType,
		typename... Args,
		typename... Policies
	> void addMethodMaybeConst(
		const char* name,
		ReturnType(Bound::*method)(Args...) const,
		Policies... policies
	) {
		addMethod<
			Signature<decltype(method), Bound, PolicyListType<Policies...>, ReturnType, Args...>,
			decltype(method)
		>(name, method);
	}

	template <typename... Args, typename... Policies>
	BindDefiner &constructor(Policies...) {
		typedef ConstructorSignature<Bound, PolicyListType<Policies...>, Args...> Signature;

		bindClass.addConstructor(&Signature::getInstance());

		return(*this);
	}

	template <typename ReturnType, typename... Args, typename... Policies>
	BindDefiner &method(
		const char* name,
		ReturnType(*func)(Args...),
		Policies... policies
	) {
		addMethod<
			FunctionSignature<decltype(func), std::nullptr_t, PolicyListType<Policies...>, ReturnType, Args...>,
			decltype(func)
		>(name, func);

		return(*this);
	}

	template <typename MethodType, typename... Policies>
	BindDefiner &method(
		const char* name,
		MethodType method,
		Policies... policies
	) {
		addMethodMaybeConst<MethodSignature>(name, method, policies...);

		return(*this);
	}

	template <typename GetterType, typename... Policies>
	BindDefiner &property(
		const char* name,
		GetterType getter,
		Policies... policies
	) {
		addMethodMaybeConst<GetterSignature>(name, getter, policies...);

		bindClass.addMethod(emptySetter);

		return(*this);
	}

	template <typename GetterType, typename SetterType, typename... Policies>
	BindDefiner &property(
		const char* name,
		GetterType getter,
		SetterType setter,
		Policies... policies
	) {
		addMethodMaybeConst<GetterSignature>(name, getter, policies...);
		addMethodMaybeConst<SetterSignature>(name, setter, policies...);

		return(*this);
	}

private:

	BindClass<Bound> &bindClass;

};

} // namespace
