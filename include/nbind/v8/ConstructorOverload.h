// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// TODO: Remove this file.

#pragma once

namespace nbind {

template <typename Bound>
class ArgStorage;

template <class Bound>
class ConstructorOverload {

public:

	// Wrapper that calls the C++ constructor when called from a
	// fromJS function written in JavaScript.

	static void valueConstructorCaller(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		auto *constructor = ConstructorOverload::getValueConstructor(args.Length());

		if(constructor == nullptr) {
			// Can't use throw here because there's V8 code
			// (lacking C++ exception support) on the stack
			// above the catch statement.
			NBIND_ERR("Wrong number of arguments in value binding");
			args.GetReturnValue().Set(Nan::Undefined());

			return;
		}

		ArgStorage<Bound> &storage = *static_cast<ArgStorage<Bound> *>(v8::Handle<v8::External>::Cast(args.Data())->Value());

		// the contents of makeValue from Creator.h belong here.
		constructor(storage, args);

		args.GetReturnValue().Set(Nan::Undefined());
	}

	template<typename MethodType> struct NanValueConstructorTypeBuilder;

	template<typename ReturnType, typename... NanArgs>
	struct NanValueConstructorTypeBuilder<ReturnType(NanArgs...)> {
		typedef void type(ArgStorage<Bound> &storage, NanArgs...);
	};

	typedef std::remove_pointer<Nan::FunctionCallback>::type jsMethod;

	struct jsConstructors {
		typedef typename NanValueConstructorTypeBuilder<jsMethod>::type valueType;

		jsConstructors(valueType *value = nullptr) : value(value) {}

		valueType *value;
	};

	// Store link to constructor, possibly overloaded by arity.
	// It will be declared with the Node API when this module is initialized.

	static void addConstructor(unsigned int arity, typename jsConstructors::valueType *funcValue) {
		static std::vector<jsConstructors> &constructorVect = constructorVectStore();
		signed int oldArity = getArity();

		if(signed(arity) > oldArity) constructorVect.resize(arity + 1);

		constructorVect[arity].value = funcValue;
	}

	// Get maximum arity among overloaded constructors.
	// Can be -1 if there are no constructors.

	static signed int getArity() {
		return(constructorVectStore().size() - 1);
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
};

} // namespace
