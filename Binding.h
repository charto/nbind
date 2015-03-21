// This file is part of nbind, copyright (C) 2014 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include <forward_list>
#include <vector>

#include <v8.h>
#include <node.h>
#include <nan.h>

#include "BindingType.h"

#include "wire.h"

namespace nbind {

inline static NAN_METHOD(dummyMethod) {NanReturnNull();}
typedef decltype(dummyMethod) jsMethod;

class BindClassBase;

class Bindings {

public:

	static void registerClass(BindClassBase *bindClass);
	static void initModule(v8::Handle<v8::Object> exports);

private:

	static std::forward_list<BindClassBase *> &getClassList() {
		static std::forward_list<BindClassBase *> classList;
		return(classList);
	}

};

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

	typedef v8::Persistent<v8::Function> &getConstructorType();

	std::forward_list<methodDef> &getMethodList() {return(methodList);}

	jsMethod *createPtr;
	getConstructorType *getConstructorPtr;

protected:

	const char *name;
	std::forward_list<methodDef> methodList;

};

template <class Bound> class BindClass : public BindClassBase {

public:

	BindClass():BindClassBase() {
		createPtr=create;
		getConstructorPtr=getConstructor;
		setInstance(this);
	}

	typedef BindWrapper<Bound> *jsConstructor(const v8::Arguments &args);

	void addConstructor(int arity,jsConstructor *ptr) {
		static std::vector<jsConstructor *> &constructorTbl=constructorTblStore();
		int oldArity=getArity();

		if(arity>oldArity) {
			constructorTbl.resize(arity+1);
			for(int pos=oldArity+1;pos<arity;pos++) {
				constructorTbl[pos]=nullptr;
			}
		}

		constructorTbl[arity]=ptr;
	}

	static int getArity() {
		return(constructorTblStore().size()-1);
	}

	static jsConstructor *getConstructor(unsigned int arity) {
		return(constructorTblStore()[arity]);
	}

//	static v8::Handle<v8::Value> create(const v8::Arguments &args);
	static NAN_METHOD(create);

	static v8::Persistent<v8::Function> &getConstructor() {
		static v8::Persistent<v8::Function> constructor;
		return(constructor);
	}

	static BindClass *getInstance() {return(instanceStore());}
	void setInstance(BindClass *instance) {instanceStore()=instance;}

	static BindClass *&instanceStore() {
		static BindClass *instance;
		return(instance);
	}

	static std::vector<jsConstructor *> &constructorTblStore() {
		static std::vector<jsConstructor *> constructorTbl;
		return(constructorTbl);
	}

};

template<typename...> struct TypeList {};

template<typename ReturnType,typename ArgList> struct Caller;

template<typename ReturnType,typename... Args>
struct Caller<ReturnType,TypeList<Args...>> {
	template <class Bound,typename Method>
	static v8::Handle<v8::Value> call(Bound &target,Method method,const v8::Arguments &args) {
		return(BindingType<ReturnType>::toWireType(
			(target.*method)(Args::get(args)...)
		));
	}
};

// Specialize Caller for void return type, because called function has no
// return value to toWireType.
template<typename... Args>
struct Caller<void,TypeList<Args...>> {
	template <class Bound,typename Method>
	static v8::Handle<v8::Value> call(Bound &target,Method method,const v8::Arguments &args) {
		(target.*method)(Args::get(args)...);
		NanReturnUndefined();
	}
};

template<size_t Index,typename ArgType>
struct FromWire {
	typedef struct {
//		static u32 get(const v8::Arguments &args) {
//			return(node::Buffer::HasInstance(args[Index]));
//		}
		static ArgType get(const v8::Arguments &args) {
			return(BindingType<ArgType>::fromWireType(args[Index]));
		}
	} type;
};

// Templated static class for each possible different method call exposed by the
// Node.js plugin. Used to pass arguments and return values between C++ and Node.js.
// Everything must be static because the V8 JavaScript engine wants a single
// function pointer to call.

template <class Bound,typename ReturnType,typename... Args>
class MethodInfo {

public:

	typedef ReturnType(Bound::*MethodType)(Args...);

	static MethodType getMethod() {return(methodStore());}
	static const char *getClassName() {return(classNameStore());}
	static const char *getMethodName() {return(methodNameStore());}
	static void setMethod(MethodType method) {methodStore()=method;}
	static void setClassName(const char *className) {classNameStore()=className;}
	static void setMethodName(const char *methodName) {methodNameStore()=strdup(methodName);}

	static NAN_METHOD(call) {
		NanEscapableScope();
		static constexpr decltype(args.Length()) arity=sizeof...(Args);

		if(args.Length()!=arity) {
//			printf("Wrong number of arguments to %s.%s: expected %ld, got %d.\n",getClassName(),getMethodName(),arity,args.Length());
			NanThrowError("Wrong number of arguments");
			NanReturnNull();
		}

		v8::Local<v8::Object> targetWrapped=args.This();
		Bound &target=node::ObjectWrap::Unwrap<BindWrapper<Bound>>(targetWrapped)->bound;

		return(NanEscapeScope((Caller<
			ReturnType,
			typename emscripten::internal::MapWithIndex<
				TypeList,
				FromWire,
				Args...
			>::type
		>::call(target,getMethod(),args))));
	}

private:

	static MethodType &methodStore() {
		static MethodType method;
		return(method);
	}

	static const char *&classNameStore() {
		static const char *className;
		return(className);
	}

	static const char *&methodNameStore() {
		static const char *methodName;
		return(methodName);
	}

};

template<class Bound,typename ArgList> struct ConstructorInfo;

template<class Bound,typename... Args>
struct ConstructorInfo<Bound,TypeList<Args...>> {

public:

//	static MethodType getMethod() {return(methodStore());}
	static const char *getClassName() {return(classNameStore());}
//	static void setMethod(MethodType method) {methodStore()=method;}
	static void setClassName(const char *className) {classNameStore()=className;}

	static BindWrapper<Bound> *call(const v8::Arguments &args) {
		return(new BindWrapper<Bound>(Args::get(args)...));
	}

/*
	static v8::Handle<v8::Value> call(const v8::Arguments &args) {
		v8::HandleScope scope;
		static constexpr size_t arity=sizeof...(Args);

		if(args.Length()!=arity) {
			// TODO: Throw JavaScript exception
			printf("Wrong number of arguments to %s.%s: expected %ld, got %d.\n",getClassName(),getMethodName(),arity,args.Length());
			v8::ThrowException(v8::String::New("Wrong number of arguments"));
			return(v8::Local<v8::Value>());
		}

		v8::Local<v8::Object> targetWrapped=args.This();
		Bound &target=node::ObjectWrap::Unwrap<BindWrapper<Bound>>(targetWrapped)->bound;

		return(scope.Close(Caller<
			ReturnType,
			typename emscripten::internal::MapWithIndex<
				TypeList,
				FromWire,
				Args...
			>::type
		>::call(target,getMethod(),args)));
	}
*/
private:
/*
	static MethodType &methodStore() {
		static MethodType method;
		return(method);
	}
*/
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
		typedef MethodInfo<Bound,ReturnType,Args...> Method;

		Method::setMethod(method);
		Method::setClassName(this->name);
		Method::setMethodName(name);
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

// The create function would better fit in BindClass but it needs to call
// node::ObjectWrap::Wrap which is protected and only inherited by BindWrapper.
template <class Bound>
NAN_METHOD(BindClass<Bound>::create) {
	return(BindWrapper<Bound>::create(args));
}

template <class Bound>
NAN_METHOD(BindWrapper<Bound>::create) {
	if(args.IsConstructCall()) {
		NanScope();

		// Called like new Bound(...)
		if(args.Length()>BindClass<Bound>::getArity()) {
			NanThrowError("Wrong number of arguments");
			NanReturnNull();
		}

		auto *constructor=BindClass<Bound>::getConstructor(args.Length());

		if(constructor==nullptr) {
			NanThrowError("Wrong number of arguments");
			NanReturnNull();
		}

		constructor(args)->Wrap(args.This());

		NanReturnThis();
	} else {
		NanEscapableScope();

		// Called like Bound(...), add the "new" operator.
		unsigned int argc=args.Length();
		std::vector<v8::Handle<v8::Value>> argv(argc);

		for(unsigned int argNum=0;argNum<argc;argNum++) argv[argNum]=args[argNum];

		return(NanEscapeScope(BindClass<Bound>::getConstructor()->NewInstance(argc,&argv[0])));
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
