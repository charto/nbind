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
#include "MethodSignature.h"

#include "wire.h"

namespace nbind {

// Get type of method definitions to use in function pointers.

inline static NAN_METHOD(dummyMethod) {NanReturnNull();}
typedef decltype(dummyMethod) jsMethod;

// Templated singleton class for each C++ class accessible from Node.js.
// Stores their information defined in static constructors, until the Node.js
// plugin is initialized.

class BindClassBase {

private:

	class methodDef {

	public:

		methodDef(const char *name,jsMethod *ptr):name(name),ptr(ptr) {}
		const char *getName() {return(name);}
		jsMethod *getMethod() {return(ptr);}

	private:

		const char *name;
		jsMethod *ptr;
	};

public:

	BindClassBase() {}

	const char *getName() {return(name);}
	void setName(const char *name) {this->name=name;}

	void addMethod(const char *name,jsMethod *ptr) {
		methodList.emplace_front(name,ptr);
	}

	std::forward_list<methodDef> &getMethodList() {return(methodList);}

	jsMethod *createPtr;

protected:

	const char *name;
	std::forward_list<methodDef> methodList;

};

template <class Bound> class BindClass : public BindClassBase {

public:

	static const char *&getNameStore() {
		static const char *name;
		return(name);
	}

	BindClass():BindClassBase() {
		createPtr=create;
		setInstance(this);
	}

	// Use template magic to build a function type with argument types
	// matching NAN_METHOD and returning a C++ class wrapped inside ObjectWrap.

	template<typename MethodType> struct NanConstructorTypeBuilder;

	template<typename ReturnType, typename... NanArgs>
	struct NanConstructorTypeBuilder<ReturnType(NanArgs...)> {
		typedef BindWrapper<Bound> *type(NanArgs...);
	};

	typedef typename NanConstructorTypeBuilder<jsMethod>::type jsConstructor;

	// Store link to constructor, possibly overloaded by arity.
	// It will be declared with the Node API when this module is initialized.

	void addConstructor(unsigned int arity, jsConstructor *ptr) {
		static std::vector<jsConstructor *> &constructorTbl = constructorTblStore();
		signed int oldArity = getArity();

		if(signed(arity) > oldArity) {
			constructorTbl.resize(arity + 1);
			for(unsigned int pos = oldArity + 1; pos < arity; pos++) {
				constructorTbl[pos] = nullptr;
			}
		}

		constructorTbl[arity] = ptr;
	}

	// Get maximum arity among overloaded constructors.
	// Can be -1 if there are no constructors.

	static signed int getArity() {
		return(constructorTblStore().size() - 1);
	}

	// Get constructor by arity.
	// When called, the constructor returns an ObjectWrap.

	static jsConstructor *getConstructorWrapper(unsigned int arity) {
		// Check if constructor was called with more than the maximum number
		// of arguments it can accept.
		if(signed(arity) > getArity()) return(nullptr);

		return(constructorTblStore()[arity]);
	}

	static NAN_METHOD(create);

	// Use a static variable inside a static method to provide linkage for a
	// singleton instance of this class.

	static BindClass *getInstance() {return(instanceStore());}
	void setInstance(BindClass *instance) {instanceStore()=instance;}

	static BindClass *&instanceStore() {
		static BindClass *instance;
		return(instance);
	}

	// Linkage for a table of overloaded constructors
	// (overloads must have different arities).

	static std::vector<jsConstructor *> &constructorTblStore() {
		static std::vector<jsConstructor *> constructorTbl;
		return(constructorTbl);
	}

};

template<class Bound, typename ArgList> struct ConstructorInfo;

template<class Bound, typename... Args>
struct ConstructorInfo<Bound, TypeList<Args...>> {

public:

	static const char *getClassName() {return(classNameStore());}
	static void setClassName(const char *className) {classNameStore()=className;}

	template <typename... NanArgs>
	static BindWrapper<Bound> *call(NanArgs... args) {
		return(new BindWrapper<Bound>(Args::get(std::forward<NanArgs>(args)...)...));
	}

private:

	static const char *&classNameStore() {
		static const char *className;
		return(className);
	}

};

// BindDefiner is a helper class to make class definition syntax match embind.

class BindDefinerBase {

protected:

	BindDefinerBase(const char *name):name(strdup(name)) {}

	const char *name;

};

template <class Bound>
class BindDefiner : public BindDefinerBase {

public:

	BindDefiner(const char *name):BindDefinerBase(name) {
		bindClass=BindClass<Bound>::getInstance();
		bindClass->setName(name);

		Bindings::registerClass(bindClass);
	}

	template<typename ReturnType,typename... Args,typename... Policies>
	const BindDefiner &function(const char* name,ReturnType(Bound::*method)(Args...),Policies...) const {
		typedef MethodSignature<Bound, ReturnType, Args...> Method;

		Method::setMethod(name, method);
		Method::setClassName(this->name);
		// TODO BUG FIX: Use Method::getIndex to create a running counter of
		// all methods with the same signature.
		bindClass->addMethod(name,Method::call);

		return(*this);
	}

	template<typename... Args,typename... Policies>
	const BindDefiner &constructor(Policies...) const {
		typedef ConstructorInfo<
			Bound,
			typename emscripten::internal::MapWithIndex<
				TypeList,
				FromWire,
				Args...
			>::type
		> Constructor;

		Constructor::setClassName(this->name);
		bindClass->addConstructor(sizeof...(Args),Constructor::call);

		return(*this);
	}

private:

	BindClass<Bound> *bindClass;

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
		// Called like new Bound(...)
		NanScope();

		// Look up possibly overloaded C++ constructor according to its arity
		// in the constructor call.
		auto *constructor=BindClass<Bound>::getConstructorWrapper(args.Length());

		if(constructor==nullptr) {
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
		// Called like Bound(...), add the "new" operator.
		NanScope();

		unsigned int argc=args.Length();
		std::vector<v8::Handle<v8::Value>> argv(argc);

		// Copy arguments to a vector because the arguments object type
		// cannot be passed to another function call as-is.
		for(unsigned int argNum=0;argNum<argc;argNum++) argv[argNum]=args[argNum];

		// Find constructor function by name from a perstistent copy of the
		// module's exports object. This double lookup might be slow, but
		// calling the constructor without "new" is wrong anyway.
		v8::Local<v8::Object> constructorTbl = NanNew<v8::Object>(constructorStore);
		auto constructor = v8::Handle<v8::Function>::Cast(constructorTbl->Get(NanNew<v8::String>(BindClass<Bound>::getInstance()->getName())));

		// Call the JavaScript constructor with the new operator.
		NanReturnValue(constructor->NewInstance(argc,&argv[0]));
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
