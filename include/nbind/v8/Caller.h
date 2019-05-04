// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Caller handles the template magic to compose a method call from a class and
// parts of a method signature extracted from it.

template<typename...> struct TypeList {};

template <typename NanArgs>
v8::Local<v8::Value> makeTypeError(
	NanArgs &args,
	uint32_t count,
	const TYPEID *typeList,
	bool *flagList
) {
	(void)args; // Silence compiler warning about unused parameter.

	v8::Local<v8::Value> err = Nan::TypeError("Type mismatch");
	v8::Local<v8::Object> errObj = Nan::To<v8::Object>(err).ToLocalChecked();
	// v8::Local<v8::Array> typeArray = Nan::New<v8::Array>(count);
	v8::Local<v8::Array> flagArray = Nan::New<v8::Array>(count);

	for(uint32_t num = 0; num < count; ++num) {
		flagArray->Set(num, Nan::New<v8::Boolean>(flagList[num]));
	}

	// errObj->Set(Nan::New<v8::String>("types").ToLocalChecked(), typeArray);
	errObj->Set(Nan::New<v8::String>("flags").ToLocalChecked(), flagArray);

	return(err);
}

// CheckWire verifies if the type of a JavaScript handle corresponds to a C++ type.

template<typename PolicyList, size_t Index, typename ArgType>
struct CheckWire {

	typedef TypeTransformer<ArgType, PolicyList> Transformed;

	template <typename NanArgs>
	static inline bool checkType(const NanArgs &args) {
		return(Transformed::Binding::checkType(args[Index]));
	}

};

template<typename ArgList> struct Checker;

template<typename... Args>
struct Checker<TypeList<Args...>> {

	static bool booleanAnd(bool flag) {
		return(flag);
	}

	template <typename... Rest>
	static bool booleanAnd(bool flag, Rest... rest) {
		return(flag & booleanAnd(rest...));
	}

	template <typename NanArgs>
	static bool typesAreValid(NanArgs &args) {
		(void)args; // Silence possible compiler warning about unused parameter.

		return(booleanAnd(Args::checkType(args)..., true));
	}

	template <typename NanArgs>
	static v8::Local<v8::Value> getTypeError(NanArgs &args, const TYPEID *typeList) {
		(void)args; // Silence possible compiler warning about unused parameter.

		bool flagList[] = { Args::checkType(args)..., true };

		return(makeTypeError(args, sizeof...(Args), typeList, flagList));
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
			target,
			0.0
		));
	}

	template <typename Function, typename NanArgs>
	static WireType callFunction(Function func, NanArgs &args) noexcept(false) {
		(void)args; // Silence possible compiler warning about unused parameter.

		// Note that Args().get may throw.
		return(convertToWire<ReturnType>(
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
