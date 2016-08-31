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

template <typename PolicyList>
struct SkipNamePolicy {
	typedef PolicyList Type;
};

template <typename... Policies>
struct SkipNamePolicy<PolicyListType<const char *, Policies...>> {
	typedef PolicyListType<Policies...> Type;
};

template <typename... Policies>
const char *executeNamePolicy(const char *name, Policies... policies) {
	return(name);
}

template <typename... Policies>
const char *executeNamePolicy(const char *name, const char *boundName, Policies... policies) {
	return(boundName);
}

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
		MethodType method,
		WrapperFlags flags = WrapperFlags::none
	) {
		bindClass.addMethod(
			name,
			&Signature::getInstance(),
			Signature::getDirect(method),
			Signature::addMethod(method, flags),
			flags
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
			Signature<
				decltype(method),
				Bound,
				typename SkipNamePolicy<PolicyListType<Policies...>>::Type,
				ReturnType,
				Args...
			>,
			decltype(method)
		>(executeNamePolicy(name, policies...), method);
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
			Signature<
				decltype(method),
				Bound,
				typename SkipNamePolicy<PolicyListType<Policies...>>::Type,
				ReturnType,
				Args...
			>,
			decltype(method)
		>(
			executeNamePolicy(name, policies...),
			method,
			WrapperFlags::constant
		);
	}

	template <typename... Args, typename... Policies>
	BindDefiner &constructor(Policies...) {
		typedef ConstructorSignature<
			Bound,
			typename SkipNamePolicy<PolicyListType<Policies...>>::Type,
			Args...
		> Signature;

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
			FunctionSignature<
				decltype(func),
				std::nullptr_t,
				typename SkipNamePolicy<PolicyListType<Policies...>>::Type,
				ReturnType,
				Args...
			>,
			decltype(func)
		>(executeNamePolicy(name, policies...), func);

		return(*this);
	}

	template <typename ReturnType, typename... Args, typename... Policies>
	BindDefiner &method(
		const char* name,
		ReturnType(*func)(Args...),
		Overloaded<ReturnType(Args...)>,
		Policies... policies
	) {
		addMethod<
			FunctionSignature<
				decltype(func),
				std::nullptr_t,
				typename SkipNamePolicy<PolicyListType<Policies...>>::Type,
				ReturnType,
				Args...
			>,
			decltype(func)
		>(executeNamePolicy(name, policies...), func);

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

	template <typename MethodType, typename... Policies>
	BindDefiner &method(
		const char* name,
		MethodType (Bound::*method),
		Overloaded<MethodType>,
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
