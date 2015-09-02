// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

template <typename Bound>
class ArgStorage;

template <class Bound>
class ConstructorOverload {

public:

	static void call(const Nan::FunctionCallbackInfo<v8::Value> &args);
/*
	// Wrapper that calls the C++ constructor when called from a
	// fromJS function written in JavaScript.

	static void valueConstructorCaller(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		auto *constructor = BindClass<Bound>::getValueConstructor(args.Length());

		if(constructor == nullptr) {
			// Can't use throw here because there's V8 code
			// (lacking C++ exception support) on the stack
			// above the catch statement.
			NBIND_ERR("Wrong number of arguments in value binding");
			args.GetReturnValue().Set(Nan::Undefined());

			return;
		}

		ArgStorage<Bound> &storage = *static_cast<ArgStorage<Bound> *>(v8::Handle<v8::External>::Cast(args.Data())->Value());

		constructor(storage, args);

		args.GetReturnValue().Set(Nan::Undefined());
	}

	template<typename MethodType> struct NanWrapperConstructorTypeBuilder;
	template<typename MethodType> struct NanValueConstructorTypeBuilder;

	template<typename ReturnType, typename... NanArgs>
	struct NanWrapperConstructorTypeBuilder<ReturnType(NanArgs...)> {
		typedef BindWrapper<Bound> *type(NanArgs...);
	};

	template<typename ReturnType, typename... NanArgs>
	struct NanValueConstructorTypeBuilder<ReturnType(NanArgs...)> {
		typedef void type(ArgStorage<Bound> &storage, NanArgs...);
	};

	typedef std::remove_pointer<Nan::FunctionCallback>::type jsMethod;

	struct jsConstructors {
		typedef typename NanWrapperConstructorTypeBuilder<jsMethod>::type wrapperType;
		typedef typename NanValueConstructorTypeBuilder<jsMethod>::type valueType;

		jsConstructors(wrapperType *wrapper = nullptr, valueType *value = nullptr) : wrapper(wrapper), value(value) {}

		wrapperType *wrapper;
		valueType *value;
	};

	// Store link to constructor, possibly overloaded by arity.
	// It will be declared with the Node API when this module is initialized.

	void addConstructor(unsigned int arity, typename jsConstructors::wrapperType *funcWrapper, typename jsConstructors::valueType *funcValue) {
		static std::vector<jsConstructors> &constructorVect = constructorVectStore();
		signed int oldArity = getArity();

		if(signed(arity) > oldArity) constructorVect.resize(arity + 1);

		constructorVect[arity].wrapper = funcWrapper;
		constructorVect[arity].value = funcValue;
	}

	// Get maximum arity among overloaded constructors.
	// Can be -1 if there are no constructors.

	static signed int getArity() {
		return(constructorVectStore().size() - 1);
	}

	// Get constructor by arity.
	// When called, the constructor returns an ObjectWrap.

	static typename jsConstructors::wrapperType *getWrapperConstructor(unsigned int arity) {
		// Check if constructor was called with more than the maximum number
		// of arguments it can accept.
		if(signed(arity) > getArity()) return(nullptr);

		return(constructorVectStore()[arity].wrapper);
	}

	static typename jsConstructors::valueType *getValueConstructor(unsigned int arity) {
		// Check if constructor was called with more than the maximum number
		// of arguments it can accept.
		if(signed(arity) > getArity()) return(nullptr);

		return(constructorVectStore()[arity].value);
	}

	// Use a static variable inside a static method to provide linkage for
	// a singleton instance of this class.
	// A reference will be stored in a list of all wrapped classes,
	// so they can be initialized in initModule.

	// Linkage for a table of overloaded constructors
	// (overloads must have different arities).

	static std::vector<jsConstructors> &constructorVectStore() {
		static std::vector<jsConstructors> constructorVect;
		return(constructorVect);
	}
*/
};

} // namespace
