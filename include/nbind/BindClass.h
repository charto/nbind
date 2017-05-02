// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

class BindClassBase;

struct SuperClassSpec {
	explicit SuperClassSpec(
		BindClassBase &superClass,
		void *(&upcast)(void *)
	) : superClass(superClass), upcast(upcast) {}

	BindClassBase &superClass;
	void *(&upcast)(void *);
};

// Templated singleton class for each C++ class accessible from Node.js.
// Stores their information defined in static constructors, until the Node.js
// plugin is initialized.

class BindClassBase {

public:

	// Get type of method definitions to use in function pointers.

#if defined(BUILDING_NODE_EXTENSION)

	typedef std::remove_pointer<Nan::FunctionCallback>::type jsMethod;
	typedef std::remove_pointer<Nan::GetterCallback>::type jsGetter;
	typedef std::remove_pointer<Nan::SetterCallback>::type jsSetter;

#else

	typedef void (jsMethod)();
	typedef void (jsGetter)();
	typedef void (jsSetter)();

#endif // BUILDING_NODE_EXTENSION

	const TYPEID *getTypes() const { return(idList); }

	const char **getPolicies() const { return(policyNameList); }

	const char *getName() const { return(name); }

	std::forward_list<SuperClassSpec> &getSuperClassList() {
		return(superClassList);
	}

	unsigned int getSuperClassCount() { return(superClassCount); }

	void addConstructor(BaseSignature *signature) {

#		if defined(BUILDING_NODE_EXTENSION)

			Overloader::addMethod(
				wrapperConstructorNum,
				signature->getArity(),
				signature->getCaller()
			);

			Overloader::addMethod(
				valueConstructorNum,
				signature->getArity(),
				signature->getValueConstructor()
			);

#		endif // BUILDING_NODE_EXTENSION

		addMethod(nullptr, signature);

	}

	// Functions called when defining an nbind wrapper for a C++ class.
	// They are called from the constructor of a static variable,
	// meaning they run before any other code when the compiled library is loaded.
	// They add pointers to the wrapped C++ functions, which will be bound to
	// JavaScript prototype objects by the library's initModule function.

	// Add a method to the class.

	void addMethod(
		const char *name,
		BaseSignature *signature = nullptr,
		funcPtr ptr = nullptr,
		unsigned int num = 0,
		TypeFlags flags = TypeFlags::none
	) {
		methodList.emplace_after(methodLast, name, ptr, num, signature, flags);
		++methodLast;
	}

	std::forward_list<MethodDef> &getMethodList() { return(methodList); }

	jsMethod *getDeleter() const { return(deleter); }

#if defined(BUILDING_NODE_EXTENSION)

	unsigned int wrapperConstructorNum = Overloader::addGroup();
	unsigned int valueConstructorNum = Overloader::addGroup();

	/** Handler to wrap object pointers instantiated in C++ for use in JavaScript. */
	jsMethod *wrapPtr;

	Nan::Persistent<v8::FunctionTemplate> constructorTemplate;
	Nan::Persistent<v8::FunctionTemplate> superTemplate;
	Nan::Persistent<v8::ObjectTemplate> storageTemplate;

#endif // BUILDING_NODE_EXTENSION

	// A JavaScript "value constructor" creates a JavaScript object with
	// the same data as the equivalent C++ object. This is used for small
	// objects that should work like primitives, such as coordinate pairs.

	// Calling C++ accessor methods would be much slower than reading
	// directly from a JavaScript object. Also, using a JIT-compiled
	// constructor function directly created an object with compact and
	// fast, in-object properties:
	// http://jayconrod.com/posts/52/a-tour-of-v8-object-representation

	void setValueConstructorJS(cbFunction &func) {
		if(valueConstructorJS != nullptr) delete(valueConstructorJS);
		valueConstructorJS = new cbFunction(func);
	}

	cbFunction *getValueConstructorJS() const { return(valueConstructorJS); }

	void *upcastStep(BindClassBase &dst, void *ptr) {
		if(&dst == this) return(ptr);

		for(auto &spec : superClassList) {
			void *superPtr = spec.superClass.upcastStep(dst, spec.upcast(ptr));
			if(superPtr) return(superPtr);
		}

		return(nullptr);
	}

	bool isVisited() { return(visited); }
	void visit() { visited = true; }
	void unvisit() { visited = false; }

	bool isReady() { return(ready); }
	void setReady() { ready = true; }

protected:

	TYPEID idList[2];

	const char **policyNameList;

	const char *name;

	std::forward_list<SuperClassSpec> superClassList;
	unsigned int superClassCount = 0;

	std::forward_list<MethodDef> methodList;
	decltype(methodList.before_begin()) methodLast = methodList.before_begin();

	jsMethod *deleter;

	// This has to be a pointer instead of a member object so the
	// destructor won't get called. Otherwise NanCallback's destructor
	// segfaults when freeing V8 resources because the surrounding
	// object gets destroyed after the V8 engine.

	// Suitable JavaScript constructor called by a toJS C++ function
	// when converting this object into a plain JavaScript object,
	// if possible.

	cbFunction *valueConstructorJS = nullptr;

	bool visited = false;
	bool ready = false;

};

// Templated singleton class used for storing the definitions of a single
// wrapped C++ class.

template <class Bound> class BindClass : public BindClassBase {

public:

	void init(const char *name) {
		idList[0] = Typer<Bound>::makeID();
		idList[1] = Typer<Bound *>::makeID();

		this->name = name;
		this->policyNameList = DetectPolicies<Bound>::getPolicies();
		this->deleter = reinterpret_cast<jsMethod *>(&BindClass::destroy);
	}

	// Making the instance a direct class member might fail on some platforms.

	static BindClass &getInstance() {
		// Linkage for a singleton instance of each templated class.
		static BindClass instance;

		return(instance);
	}

	template <class SuperType>
	inline void addSuperClass();

#if defined(BUILDING_NODE_EXTENSION)

	static void destroy(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		v8::Local<v8::Object> targetWrapped = args.This();
		auto wrapper = node::ObjectWrap::Unwrap<BindWrapper<Bound>>(targetWrapped);

		wrapper->destroy();
	}

#elif defined(__EMSCRIPTEN__)

	static void destroy(uint32_t, void *ptr, void *shared, TypeFlags flags) {
		if((flags & TypeFlags::refMask) == TypeFlags::isSharedPtr) {
			delete(static_cast<std::shared_ptr<Bound> *>(shared));
		} else {
			delete(static_cast<Bound *>(ptr));
		}
	}

#endif

};

#if defined(BUILDING_NODE_EXTENSION)

template <class Bound>
BindClassBase &BindWrapper<Bound> :: getBindClass() {
	return(BindClass<Bound>::getInstance());
}

/** Test that a JavaScript object is an instance of a class from this addon
  * and has some data attached (hopefully a wrapped C++ object). */

template <class Bound>
void BindWrapper<Bound> :: testInstance(v8::Local<v8::Object> arg) {
	BindClassBase &bindClass = getBindClass();

	if(bindClass.superTemplate.IsEmpty()) {
		throw(std::runtime_error("Unbound type"));
	}

	if(
		!Nan::New(bindClass.superTemplate)->HasInstance(arg) ||
		arg->InternalFieldCount() != 1
	) {
		throw(std::runtime_error("Type mismatch"));
	}
}

template <class Bound>
Bound *BindWrapperBase :: upcast() {
	BindClassBase &src = getClass();
	BindClassBase &dst = BindClass<Bound>::getInstance();

	if(&src == &dst) return(static_cast<Bound *>(boundUnsafe));

	return(static_cast<Bound *>(src.upcastStep(dst, boundUnsafe)));
}

#endif

template <class Bound>
cbFunction *getValueConstructorJS() {
	return(BindClass<Bound>::getInstance().getValueConstructorJS());
}

template <class Bound, class SuperType>
void *upcast(void *arg) {
	return(static_cast<SuperType *>(static_cast<Bound *>(arg)));
}

template <class Bound> template <class SuperType>
inline void BindClass<Bound> :: addSuperClass() {
	superClassList.emplace_front(
		BindClass<SuperType>::getInstance(),
		upcast<Bound, SuperType>
	);
	++superClassCount;
}

} // namespace
