// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Caller handles the template magic to compose a method call from a class and
// parts of a method signature extracted from it.

template<typename...> struct TypeList {};

template<typename ArgList> struct Checker;

template<typename... Args>
struct Checker<TypeList<Args...>> {

	template<typename... DummyArgs> static inline void pass(DummyArgs&&...) {}

	static inline bool booleanAndTo(bool valid, bool &validFlag) {
		validFlag &= valid;
		return(valid);
	}

	template <typename NanArgs>
	static bool typesAreValid(NanArgs &args) {
		bool validFlag = true;
		(void)args; // Silence possible compiler warning about unused parameter.

		pass(booleanAndTo(Args::check(args), validFlag)...);

		return(validFlag);
	}

};

// This just calls BindClass<Bound>::getInstance()->getValueConstructorJS();
// It's in a wrapper function defined in BindClass.h to break a circular
// dependency between header files.

template <class Bound>
cbFunction *getValueConstructorJS();

// Handle value types as return values.
// This converter allows overriding the return type's toJS function
// with the wrapped object's toJS function.

template<typename ReturnType> struct MethodResultConverter {

	template <typename Bound>
	static inline auto toWireType(ReturnType &&result, Bound &target) -> typename std::remove_reference<decltype(
		// SFINAE, use this template only if Bound::toJS(ReturnType, cbOutput) exists.
		target.toJS(*(ReturnType *)nullptr, *(cbOutput *)nullptr),
		// Actual return type of this function: WireType (decltype adds a reference, which is removed).
		*(WireType *)nullptr
	)>::type {
		// This function is similar to BindingType<ArgType>::toWireType(ArgType &&arg).

		v8::Local<v8::Value> output = Nan::Undefined();
		cbFunction *jsConstructor = getValueConstructorJS<typename std::remove_pointer<ReturnType>::type>();

		if(jsConstructor != nullptr) {
			cbOutput construct(*jsConstructor, &output);

			target.toJS(std::move(result), construct);
		} else {
			// Throw error here?
		}

		return(output);
	}

	// If Bound::toJS(ReturnType, cbOutput) is missing, fall back to ReturnType::toJS(cbOutput).
	template <typename Bound>
	static inline WireType toWireType(ReturnType &&result, Bound &target) {
		return(BindingType<ReturnType>::toWireType(std::move(result)));
	}

};

template<typename ReturnType, typename ArgList> struct Caller;

template<typename ReturnType, typename... Args>
struct Caller<ReturnType, TypeList<Args...>> {

	template <class Bound, typename MethodType, typename NanArgs>
	static WireType callMethod(Bound &target, MethodType method, NanArgs &args) noexcept(false) {
		(void)args; // Silence possible compiler warning about unused parameter.

		// Note that Args().get may throw.
		return(MethodResultConverter<ReturnType>::toWireType(
			(target.*method)(Args(args).get(args)...),
			target
		));
	}

	template <typename Function, typename NanArgs>
	static WireType callFunction(Function func, NanArgs &args) noexcept(false) {
		(void)args; // Silence possible compiler warning about unused parameter.

		// Note that Args().get may throw.
		return(BindingType<ReturnType>::toWireType(
			(*func)(Args(args).get(args)...)
		));
	}

};

// Specialize Caller for void return type, because toWireType needs a non-void
// argument.

template<typename... Args>
struct Caller<void, TypeList<Args...>> {

	template <class Bound, typename MethodType, typename NanArgs>
	static WireType callMethod(Bound &target, MethodType method, NanArgs &args) noexcept(false) {
		(void)args; // Silence possible compiler warning about unused parameter.

		// Note that Args().get may throw.
		(target.*method)(Args(args).get(args)...);

		return(Nan::Undefined());
	}

	template <typename Function, typename NanArgs>
	static WireType callFunction(Function func, NanArgs &args) noexcept(false) {
		(void)args; // Silence possible compiler warning about unused parameter.

		// Note that Args().get may throw.
		(*func)(Args(args).get(args)...);

		return(Nan::Undefined());
	}

};

} // namespace
