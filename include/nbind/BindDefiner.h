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

#ifdef BUILDING_NODE_EXTENSION

#include "v8/Caller.h"
#include "signature/BaseSignature.h"
#include "v8/ArgStorage.h"
#include "v8/Overloader.h" // Needs ArgStorage members
#include "BindClass.h"     // Needs Overloader members
#include "v8/ValueObj.h"   // Needs BindClass members
#include "v8/Creator.h"    // Needs ArgStorage members
#include "v8/Binding.h"

#elif EMSCRIPTEN

#include "signature/BaseSignature.h"
#include "BindClass.h"
#include "em/Creator.h"
#include "em/Binding.h"

#endif

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
		bindClass.setName(name);

		Bindings::registerClass(bindClass);
	}

	template<class Signature, typename MethodType>
	void addMethod(const char *name, funcPtr ptr, MethodType method) const {
		bindClass.addMethod(
			name,
			ptr,
			Signature::addMethod(method),
			&Signature::getInstance()
		);
	}

	template<typename ReturnType, typename... Args, typename... Policies>
	const BindDefiner &function(
		const char* name,
		ReturnType(Bound::*method)(Args...),
		Policies...
	) const {
		addMethod<MethodSignature<Bound, ReturnType, Args...>>(name, nullptr, method);

		return(*this);
	}

	template<typename ReturnType, typename... Args, typename... Policies>
	const BindDefiner &function(
		const char* name,
		ReturnType(*func)(Args...),
		Policies...
	) const {
		addMethod<FunctionSignature<ReturnType, Args...>>(name, reinterpret_cast<funcPtr>(func), func);

		return(*this);
	}

	template<typename... Args, typename... Policies>
	const BindDefiner &constructor(Policies...) const {
		typedef ConstructorSignature<Bound, Args...> Signature;

		bindClass.addConstructor(&Signature::getInstance());

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
		addMethod<GetterSignature<Bound, FieldType>>(name, nullptr, getter);

		bindClass.addMethod(emptySetter);

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
		addMethod<GetterSignature<Bound, GetFieldType>>(name, nullptr, getter);

		addMethod<SetterSignature<Bound, SetReturnType, SetFieldType>>(name, nullptr, setter);

		return(*this);
	}

private:

	BindClass<Bound> &bindClass;

};

} // namespace
