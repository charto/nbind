// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// TODO: This file is still ugly.
// The constructor stuff from here and Binding.h should be moved to ConstructorOverload.h.

#pragma once

namespace nbind {

// Templated singleton class for each C++ class accessible from Node.js.
// Stores their information defined in static constructors, until the Node.js
// plugin is initialized.

class BindClassBase {

public:

	// Get type of method definitions to use in function pointers.

#ifdef BUILDING_NODE_EXTENSION
	typedef std::remove_pointer<Nan::FunctionCallback>::type jsMethod;
	typedef std::remove_pointer<Nan::GetterCallback>::type jsGetter;
	typedef std::remove_pointer<Nan::SetterCallback>::type jsSetter;
#else
	typedef void (*jsMethod)();
	typedef void (*jsGetter)();
	typedef void (*jsSetter)();
#endif // BUILDING_NODE_EXTENSION

	BindClassBase() {}

	TYPEID getTypeID() { return(id); }

	const char *getName() { return(name); }
	void setName(const char *name) { this->name = name; }

	bool isReady() { return(readyState & 1); }
	bool isDuplicate() { return(readyState & 2); }

	void setDuplicate() { readyState |= 2; }

	void init() { readyState |= 1; }

	// Functions called when defining an nbind wrapper for a C++ class.
	// They are called from the constructor of a static variable,
	// meaning they run before any other code when the compiled library is loaded.
	// They add pointers to the wrapped C++ functions, which will be bound to
	// JavaScript prototype objects by the library's initModule function.

	// Add a method to the class.

	void addMethod(const char *name, funcPtr ptr = nullptr, unsigned int num = 0, BaseSignature *signature = nullptr) {
		methodList.emplace_front(name, ptr, num, signature);
	}

	std::forward_list<MethodDef> &getMethodList() {return(methodList);}

	jsMethod *createPtr;

#ifdef BUILDING_NODE_EXTENSION
	void setConstructorHandle(v8::Local<v8::Function> func) {
		if(jsConstructorHandle == nullptr) {
			jsConstructorHandle = new Nan::Callback(func);
		} else {
			jsConstructorHandle->SetFunction(func);
		}
	}

	v8::Handle<v8::Function> getConstructorHandle() {
		return(jsConstructorHandle->GetFunction());
	}

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

	cbFunction *getValueConstructorJS() {
		return(valueConstructorJS);
	}
#endif // BUILDING_NODE_EXTENSION

protected:

	TYPEID id;

	int readyState = 0;

	const char *name;

	std::forward_list<MethodDef> methodList;

	// These have to be pointers instead of a member objects so the
	// destructor won't get called. Otherwise NanCallback's destructor
	// segfaults when freeing V8 resources because the surrounding
	// object gets destroyed after the V8 engine.

#ifdef BUILDING_NODE_EXTENSION
	// Constructor called by JavaScript's "new" operator.
	Nan::Callback *jsConstructorHandle = nullptr;

	// Suitable JavaScript constructor called by a toJS C++ function
	// when converting this object into a plain JavaScript object,
	// if possible.
	cbFunction *valueConstructorJS = nullptr;
#endif // BUILDING_NODE_EXTENSION

};

// Templated singleton class used for storing the definitions of a single
// wrapped C++ class.

template <class Bound> class BindClass : public BindClassBase {

public:

	BindClass() : BindClassBase() {
		this->id = makeTypeID<Bound>();
#ifdef BUILDING_NODE_EXTENSION
		createPtr = ConstructorOverload<Bound>::call;
#elif EMSCRIPTEN
#endif
		setInstance(this);
	}
	static BindClass *getInstance() {return(instanceStore());}

	static BindClass *&instanceStore() {
		static BindClass *instance;
		return(instance);
	}

private:

	// Function called by the constructor, to permanently store a single
	// object of this singleton class.

	void setInstance(BindClass *instance) {instanceStore() = instance;}

};

} // namespace
