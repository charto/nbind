// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include <forward_list>
#include <vector>

#include <v8.h>
#include <node.h>
#include <nan.h>

#include "BindingType.h"

namespace nbind {

class BindClassBase;

// Storage for class bindings, populated by BindDefiners created in
// constructors of static BindInvoker objects.

class Bindings {

public:

	static void registerClass(BindClassBase *bindClass);
	static void initModule(v8::Handle<v8::Object> exports);

	static inline char *getError() {return(message);}
	static inline void clearError() {Bindings::message = nullptr;}
	static inline void setError(char *message) {
		if(!Bindings::message) Bindings::message = message;
	}

private:

	static char *message;

	static std::forward_list<BindClassBase *> &getClassList() {
		static std::forward_list<BindClassBase *> classList;
		return(classList);
	}

};

} // namespace

#include "wire.h"
#include "Caller.h"
#include "BindClass.h"
#include "BindDefiner.h"

namespace nbind {

template<class Bound, typename... Args>
struct ConstructorInfo<Bound, TypeList<Args...>> {

public:

	static const char *getClassName() {return(classNameStore());}
	static void setClassName(const char *className) {classNameStore() = className;}

	template <typename... NanArgs>
	static BindWrapper<Bound> *call(NanArgs... args) {
		return(new BindWrapper<Bound>(Args(std::forward<NanArgs>(args)...).get()...));
	}

private:

	static const char *&classNameStore() {
		static const char *className;
		return(className);
	}

};

extern v8::Persistent<v8::Object> constructorStore;

// The create function would better fit in BindClass but it needs to call
// node::ObjectWrap::Wrap which is protected and only inherited by BindWrapper.
template <class Bound>
NAN_METHOD(BindClass<Bound>::create) {
	return(BindWrapper<Bound>::create(args));
}

template <class Bound>
NAN_METHOD(BindWrapper<Bound>::create) {
	if(args.IsConstructCall()) {
		// Constructor was called like new Bound(...)
		NanScope();

		// Look up possibly overloaded C++ constructor according to its arity
		// in the constructor call.
		auto *constructor = BindClass<Bound>::getConstructorWrapper(args.Length());

		if(constructor == nullptr) {
			return(NanThrowError("Wrong number of arguments"));
		}

		Bindings::clearError();

		// Call C++ constructor and bind the resulting object
		// to the new JavaScript object being created.
		constructor(args)->Wrap(args.This());

		char *message = Bindings::getError();

		if(message) return(NanThrowError(message));

		NanReturnThis();
	} else {
		// Constructor was called like Bound(...), add the "new" operator.
		NanScope();

		unsigned int argc = args.Length();
		std::vector<v8::Handle<v8::Value>> argv(argc);

		// Copy arguments to a vector because the arguments object type
		// cannot be passed to another function call as-is.
		for(unsigned int argNum = 0; argNum < argc; argNum++) {
			argv[argNum] = args[argNum];
		}

		// Find constructor function by name from a perstistent copy of the
		// module's exports object. This double lookup might be slow, but
		// calling the constructor without "new" is wrong anyway.
		v8::Local<v8::Object> constructorTbl = NanNew<v8::Object>(constructorStore);
		auto constructor = v8::Handle<v8::Function>::Cast(
			constructorTbl->Get(
				NanNew<v8::String>(BindClass<Bound>::getInstance()->getName())
			)
		);

		// Call the JavaScript constructor with the new operator.
		NanReturnValue(constructor->NewInstance(argc, &argv[0]));
	}
}

// For embind compatibility.
#define class_ BindDefiner
struct allow_raw_pointers {};

// Use the constructor of a dummy global object to register bindings for a
// class before Node.js module initialization gets run.
#define NODEJS_BINDINGS(Bound) \
	BindClass<Bound> bindClass##Bound; \
	static struct BindInvoker##Bound {BindInvoker##Bound(BindClass<Bound> &bindClass);} bindInvoker##Bound(bindClass##Bound); \
	BindInvoker##Bound::BindInvoker##Bound(BindClass<Bound> &bindClass)

} // namespace
