// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

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

	const char *getName() const { return(name); }

	// The explicit nonzero test silences a compiler warning in Visual Studio.
	bool isReady() const { return((readyState & 1) != 0); }
	bool isDuplicate() const { return((readyState & 2) != 0); }

	void setDuplicate() { readyState |= 2; }

	void init() { readyState |= 1; }

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

#		elif defined(EMSCRIPTEN)

			methodList.emplace_front(nullptr, nullptr, 0, signature);

#		endif // BUILDING_NODE_EXTENSION, EMSCRIPTEN

	}

	// Functions called when defining an nbind wrapper for a C++ class.
	// They are called from the constructor of a static variable,
	// meaning they run before any other code when the compiled library is loaded.
	// They add pointers to the wrapped C++ functions, which will be bound to
	// JavaScript prototype objects by the library's initModule function.

	// Add a method to the class.

	void addMethod(const char *name, funcPtr ptr = nullptr, unsigned int num = 0, BaseSignature *signature = nullptr) {
		methodList.emplace_front(name, ptr, num, signature);
	}

	std::forward_list<MethodDef> &getMethodList() { return(methodList); }

	jsMethod *getDeleter() const { return(deleter); }

#if defined(BUILDING_NODE_EXTENSION)

	unsigned int wrapperConstructorNum = Overloader::addGroup();
	unsigned int valueConstructorNum = Overloader::addGroup();

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

protected:

	TYPEID idList[3];

	int readyState = 0;

	const char *name;

	std::forward_list<MethodDef> methodList;

	jsMethod *deleter;

	// This has to be a pointer instead of a member object so the
	// destructor won't get called. Otherwise NanCallback's destructor
	// segfaults when freeing V8 resources because the surrounding
	// object gets destroyed after the V8 engine.

	// Suitable JavaScript constructor called by a toJS C++ function
	// when converting this object into a plain JavaScript object,
	// if possible.

	cbFunction *valueConstructorJS = nullptr;

};

// Templated singleton class used for storing the definitions of a single
// wrapped C++ class.

template <class Bound> class BindClass : public BindClassBase {

public:

	void init(const char *name) {
		idList[0] = Typer<Bound>::makeID();
		idList[1] = Typer<Bound *>::makeID();
		idList[2] = Typer<const Bound *>::makeID();

		this->name = name;
		this->deleter = reinterpret_cast<jsMethod *>(&BindClass::destroy);
	}

	// Making the instance a direct class member might fail on some platforms.

	static BindClass &getInstance() {
		// Linkage for a singleton instance of each templated class.
		static BindClass instance;

		return(instance);
	}

#if defined(BUILDING_NODE_EXTENSION)

	static void destroy(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		v8::Local<v8::Object> targetWrapped = args.This();
		auto wrapper = node::ObjectWrap::Unwrap<BindWrapper<Bound>>(targetWrapped);

		wrapper->destroy();
	}

#elif defined(EMSCRIPTEN)

	static void destroy(uint32_t, Bound *obj) {
		delete(obj);
	}

#endif

};

template <class Bound>
cbFunction *getValueConstructorJS() {
	return(BindClass<Bound>::getInstance()->getValueConstructorJS());
}

} // namespace
