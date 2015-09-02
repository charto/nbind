// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

template <typename Bound>
class ArgStorage;

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

	// Storage format for method definitions.

	class MethodDef {

	public:

		enum class Type {function, method, getter, setter};

		MethodDef(Type type, const char *name, unsigned int num, funcPtr caller) :
			type(type), name(name), num(num), caller(caller) {}

		const char *getName() {return(name);}
		unsigned int getNum() {return(num);}
		funcPtr getCaller() {return(caller);}
		Type getType() {return(type);}

	private:

		Type type;
		const char *name;
		// Index to distinguish between functions with identical signatures.
		unsigned int num;
		// Signature represents return and argument types.
		funcPtr caller;

	};

	// Storage format for getter and setter definitions.

	class AccessorDef {

	public:

		AccessorDef(const char *name, unsigned int getterNum, unsigned int setterNum, funcPtr getterCaller, funcPtr setterCaller) :
			name(name), getterNum(getterNum), setterNum(setterNum), getterCaller(getterCaller), setterCaller(setterCaller) {}

		const char *getName() {return(name);}
		unsigned int getGetterNum() {return(getterNum);}
		unsigned int getSetterNum() {return(setterNum);}
		funcPtr getGetterCaller() {return(getterCaller);}
		funcPtr getSetterCaller() {return(setterCaller);}

	private:

		const char *name;
		unsigned int getterNum;
		unsigned int setterNum;
		funcPtr getterCaller;
		funcPtr setterCaller;

	};

	BindClassBase() {}

	const char *getName() {return(name);}
	void setName(const char *name) {this->name = name;}

	bool isReady() {return(ready);}

	void init() {ready = 1;}

	// Functions called when defining an nbind wrapper for a C++ class.
	// They are called from the constructor of a static variable,
	// meaning they run before any other code when the compiled library is loaded.
	// They add pointers to the wrapped C++ functions, which will be bound to
	// JavaScript prototype objects by the library's initModule function.

	// Add a method to the class.

	MethodDef &addMethod(MethodDef::Type type, const char *name, unsigned int num, funcPtr caller) {
		methodList.emplace_front(type, name, num, caller);
		return(methodList.front());
	}

	// Add handlers for getting or setting a property of the class from JavaScript.

	void addAccessor(
		const char *name,
		unsigned int getterNum,
		unsigned int setterNum,
		funcPtr getterCaller,
		funcPtr setterCaller
	) {
		accessList.emplace_front(name, getterNum, setterNum, getterCaller, setterCaller);
	}

	std::forward_list<MethodDef> &getMethodList() {return(methodList);}

	std::forward_list<AccessorDef> &getAccessorList() {return(accessList);}

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

	bool ready = 0;
	const char *name;
	std::forward_list<MethodDef> methodList;
	std::forward_list<AccessorDef> accessList;

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
#ifdef BUILDING_NODE_EXTENSION
		createPtr = BindWrapper<Bound>::create;
#endif // BUILDING_NODE_EXTENSION
		setInstance(this);
	}
#ifdef BUILDING_NODE_EXTENSION
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
#endif // BUILDING_NODE_EXTENSION
	static BindClass *getInstance() {return(instanceStore());}

	static BindClass *&instanceStore() {
		static BindClass *instance;
		return(instance);
	}

	// Linkage for a table of overloaded constructors
	// (overloads must have different arities).

#ifdef BUILDING_NODE_EXTENSION
	static std::vector<jsConstructors> &constructorVectStore() {
		static std::vector<jsConstructors> constructorVect;
		return(constructorVect);
	}
#endif // BUILDING_NODE_EXTENSION

private:

	// Function called by the constructor, to permanently store a single
	// object of this singleton class.

	void setInstance(BindClass *instance) {instanceStore() = instance;}

};

} // namespace
