// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#ifdef BUILDING_NODE_EXTENSION

#define NBIND 1

namespace nbind {
	typedef void (*funcPtr)();
}

#include <type_traits>
#include <forward_list>
#include <vector>
#include <stdexcept>

#include <v8.h>
#include <node.h>
#include <nan.h>

// Macro to report an error when exceptions are not available.

#define NBIND_ERR(message) nbind::Bindings::setError(message)

#include "BindingType.h"

namespace nbind {

class BindClassBase;

// Storage for class bindings, populated by BindDefiners created in
// constructors of static BindInvoker objects.

class Bindings {

public:

	static void registerClass(BindClassBase *bindClass);
	static void initModule(v8::Handle<v8::Object> exports);
	static void setValueConstructorByName(const char *name, cbFunction &func);

	static inline const char *getError() {return(message);}
	static inline void clearError() {Bindings::message = nullptr;}
	static inline void setError(const char *message) {
		if(!Bindings::message) Bindings::message = message;
	}

private:

	static const char *message;

	// Linkage for a list of all C++ class wrappers.

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

extern v8::Persistent<v8::Object> constructorStore;

// The create function would better fit in BindClass but it needs to call
// node::ObjectWrap::Wrap which is protected and only inherited by BindWrapper.
//template <class Bound>
//NAN_METHOD(BindClass<Bound>::create) {
//	return(BindWrapper<Bound>::create(args));
//}

template <class Bound>
NAN_METHOD(BindWrapper<Bound>::create) {
	if(args.IsConstructCall()) {
		// Constructor was called like new Bound(...)
		NanScope();

		// Look up possibly overloaded C++ constructor according to its arity
		// in the constructor call.
		auto *constructor = BindClass<Bound>::getWrapperConstructor(args.Length());

		if(constructor == nullptr) {
			return(NanThrowError("Wrong number of arguments"));
		}

		Bindings::clearError();

		// Call C++ constructor and bind the resulting object
		// to the new JavaScript object being created.
		try {
			constructor(args)->Wrap(args.This());

			const char *message = Bindings::getError();

			if(message) return(NanThrowError(message));

			NanReturnThis();
		} catch(const std::exception &ex) {
			const char *message = Bindings::getError();

			if(message == nullptr) message = ex.what();

			return(NanThrowError(message));
		}
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

		v8::Handle<v8::Function> constructor = BindClass<Bound>::getInstance()->getConstructorHandle();

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

#else // !BUILDING_NODE_EXTENSION

#define NBIND_ERR(message)

#endif // BUILDING_NODE_EXTENSION
