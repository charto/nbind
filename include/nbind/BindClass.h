// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Wrapper for C++ objects converted from corresponding JavaScript objects.
// Needed for allocating an empty placeholder object before JavaScript calls
// its constructor. See:
// http://stackoverflow.com/questions/31091223/placement-new-return-by-value-and-safely-dispose-temporary-copies

template <typename Bound>
class ArgStorage {

public:

	~ArgStorage() {
		if(isValid) reinterpret_cast<Bound *>(&data)->~Bound();
		isValid = false;
	}

	template <typename... Args>
	void init(Args&&... args) {
		::new(&data) Bound(std::forward<Args>(args)...);
		isValid = true;
	}

	Bound getBound() {
		return(std::move(*reinterpret_cast<Bound *>(&data)));
	}

private:

	typename std::aligned_storage<sizeof(Bound), alignof(Bound)>::type data;

	bool isValid = false;

};

template<class Bound, typename ArgList> struct ConstructorInfo;

template<class Bound, typename... Args>
struct ConstructorInfo<Bound, TypeList<Args...>> {

public:

	static const char *getClassName() {return(classNameStore());}
	static void setClassName(const char *className) {classNameStore() = className;}

	// Make sure prototype matches NanWrapperConstructorTypeBuilder!
	template <typename... NanArgs>
	static BindWrapper<Bound> *makeWrapper(NanArgs... args) {
		return(new BindWrapper<Bound>(Args(std::forward<NanArgs>(args)...).get()...));
	}

	// Old code that returned a constructed object by value:
	// template <typename... NanArgs>
	// static Bound makeValue(ArgStorage<Bound> &wrapper, NanArgs... args) {
	//	return(Bound(Args(std::forward<NanArgs>(args)...).get()...));
	// }

	// Make sure prototype matches NanValueConstructorTypeBuilder!
	template <typename... NanArgs>
	static void makeValue(ArgStorage<Bound> &storage, NanArgs... args) {
		storage.init(Args(std::forward<NanArgs>(args)...).get()...);
	}

private:

	static const char *&classNameStore() {
		static const char *className;
		return(className);
	}

};

// Templated singleton class for each C++ class accessible from Node.js.
// Stores their information defined in static constructors, until the Node.js
// plugin is initialized.

class BindClassBase {

protected:

	// Get type of method definitions to use in function pointers.

	inline static NAN_METHOD(dummyMethod) {NanReturnNull();}
	inline static NAN_GETTER(dummyGetter) {NanReturnNull();}
	inline static NAN_SETTER(dummySetter) {}
	typedef decltype(dummyMethod) jsMethod;
	typedef decltype(dummyGetter) jsGetter;
	typedef decltype(dummySetter) jsSetter;

public:

	// Storage format for method definitions.

	class MethodDef {

	public:

		MethodDef(const char *name, unsigned int num, jsMethod *signature) :
			name(name), num(num), signature(signature) {}

		const char *getName() {return(name);}
		unsigned int getNum() {return(num);}
		jsMethod *getSignature() {return(signature);}

	private:

		const char *name;
		// Index to distinguish between functions with identical signatures.
		unsigned int num;
		// Signature represents return and argument types.
		jsMethod *signature;

	};

	// Storage format for getter and setter definitions.

	class AccessorDef {

	public:

		AccessorDef(const char *name, unsigned int getterNum, unsigned int setterNum, jsGetter *getterSignature, jsSetter *setterSignature) :
			name(name), getterNum(getterNum), setterNum(setterNum), getterSignature(getterSignature), setterSignature(setterSignature) {}

		const char *getName() {return(name);}
		unsigned int getGetterNum() {return(getterNum);}
		unsigned int getSetterNum() {return(setterNum);}
		jsGetter *getGetterSignature() {return(getterSignature);}
		jsSetter *getSetterSignature() {return(setterSignature);}

	private:

		const char *name;
		unsigned int getterNum;
		unsigned int setterNum;
		jsGetter *getterSignature;
		jsSetter *setterSignature;

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

	void addMethod(const char *name, unsigned int num, jsMethod *signature) {
		methodList.emplace_front(name, num, signature);
	}

	// Add a static method to the class.

	void addFunction(const char *name, unsigned int num, jsMethod *signature) {
		funcList.emplace_front(name, num, signature);
	}

	// Add handlers for getting or setting a property of the class from JavaScript.

	void addAccessor(
		const char *name,
		unsigned int getterNum,
		unsigned int setterNum,
		jsGetter *getterSignature,
		jsSetter *setterSignature
	) {
		accessList.emplace_front(name, getterNum, setterNum, getterSignature, setterSignature);
	}

	std::forward_list<MethodDef> &getMethodList() {return(methodList);}

	std::forward_list<MethodDef> &getFunctionList() {return(funcList);}

	std::forward_list<AccessorDef> &getAccessorList() {return(accessList);}

	jsMethod *createPtr;

	void setConstructorHandle(v8::Handle<v8::Function> func) {
		if(jsConstructorHandle == nullptr) {
			jsConstructorHandle = new NanCallback(func);
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

protected:

	bool ready = 0;
	const char *name;
	std::forward_list<MethodDef> methodList;
	std::forward_list<MethodDef> funcList;
	std::forward_list<AccessorDef> accessList;

	// These have to be pointers instead of a member objects so the
	// destructor won't get called. Otherwise NanCallback's destructor
	// segfaults when freeing V8 resources because the surrounding
	// object gets destroyed after the V8 engine.

	// Constructor called by JavaScript's "new" operator.
	NanCallback *jsConstructorHandle = nullptr;

	// Suitable JavaScript constructor called by a toJS C++ function
	// when converting this object into a plain JavaScript object,
	// if possible.
	cbFunction *valueConstructorJS = nullptr;

};

// Templated singleton class used for storing the definitions of a single
// wrapped C++ class.

template <class Bound> class BindClass : public BindClassBase {

public:

	BindClass() : BindClassBase() {
		createPtr = create;
		setInstance(this);
	}

	// Wrapper that calls the C++ constructor when called from a
	// fromJS function written in JavaScript.

	static NAN_METHOD(valueConstructorCaller) {
		auto *constructor = BindClass<Bound>::getValueConstructor(args.Length());

//		if(constructor == nullptr) {
//			return(NanThrowError("Wrong number of arguments in value binding"));
//		}

		ArgStorage<Bound> &storage = *static_cast<ArgStorage<Bound> *>(v8::Handle<v8::External>::Cast(args.Data())->Value());

		constructor(storage, args);
	}

	// TODO: is there something to replace ??? on these lines?
	// typedef decltype(&ConstructorInfo<Bound, ???>::makeWrapper) jsWrapperConstructor;
	// typedef decltype(&ConstructorInfo<Bound, ???>::makeValue) jsValueConstructor;
	// Until then, use type builder helper classes to create the typedefs.

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

	typedef typename NanWrapperConstructorTypeBuilder<jsMethod>::type jsWrapperConstructor;
	typedef typename NanValueConstructorTypeBuilder<jsMethod>::type jsValueConstructor;

	// Store link to constructor, possibly overloaded by arity.
	// It will be declared with the Node API when this module is initialized.

// TODO: clean up duplicated code!
	void addConstructor(unsigned int arity, jsWrapperConstructor *funcWrapper, jsValueConstructor *funcValue) {
		static std::vector<jsWrapperConstructor *> &constructorVect = wrapperConstructorVectStore();
		signed int oldArity = getArity();

		if(signed(arity) > oldArity) {
			constructorVect.resize(arity + 1);
			for(unsigned int pos = oldArity + 1; pos < arity; pos++) {
				constructorVect[pos] = nullptr;
			}
		}

		constructorVect[arity] = funcWrapper;




		static std::vector<jsValueConstructor *> &constructorVect2 = valueConstructorVectStore();

		if(signed(arity) > oldArity) {
			constructorVect2.resize(arity + 1);
			for(unsigned int pos = oldArity + 1; pos < arity; pos++) {
				constructorVect2[pos] = nullptr;
			}
		}

		constructorVect2[arity] = funcValue;
	}

	// Get maximum arity among overloaded constructors.
	// Can be -1 if there are no constructors.

	static signed int getArity() {
		return(wrapperConstructorVectStore().size() - 1);
	}

	// Get constructor by arity.
	// When called, the constructor returns an ObjectWrap.

	static jsWrapperConstructor *getWrapperConstructor(unsigned int arity) {
		// Check if constructor was called with more than the maximum number
		// of arguments it can accept.
		if(signed(arity) > getArity()) return(nullptr);

		return(wrapperConstructorVectStore()[arity]);
	}

	static jsValueConstructor *getValueConstructor(unsigned int arity) {
		// Check if constructor was called with more than the maximum number
		// of arguments it can accept.
		if(signed(arity) > getArity()) return(nullptr);

		return(valueConstructorVectStore()[arity]);
	}

	static NAN_METHOD(create);

	// Use a static variable inside a static method to provide linkage for
	// a singleton instance of this class.
	// A reference will be stored in a list of all wrapped classes,
	// so they can be initialized in initModule.

	static BindClass *getInstance() {return(instanceStore());}

	static BindClass *&instanceStore() {
		static BindClass *instance;
		return(instance);
	}

	// Linkage for a table of overloaded constructors
	// (overloads must have different arities).

	static std::vector<jsWrapperConstructor *> &wrapperConstructorVectStore() {
		static std::vector<jsWrapperConstructor *> constructorVect;
		return(constructorVect);
	}

	static std::vector<jsValueConstructor *> &valueConstructorVectStore() {
		static std::vector<jsValueConstructor *> constructorVect;
		return(constructorVect);
	}

private:

	// Function called by the constructor, to permanently store a single
	// object of this singleton class.

	void setInstance(BindClass *instance) {instanceStore() = instance;}

};

// Call the toJS method of a returned C++ object, to convert it into a JavaScript object.
// This is used when a C++ function is called from JavaScript.
// A functor capable of calling the correct JavaScript constructor is passed to toJS,
// which must call the functor with arguments in the correct order.
// The functor calls the JavaScript constructor and writes a pointer to the resulting object
// directly into a local handle called "output" which is returned to JavaScript.

template <typename ArgType>
inline WireType BindingType<ArgType>::toWireType(ArgType arg) {
	v8::Local<v8::Value> output = NanUndefined();
	cbFunction *jsConstructor = BindClass<ArgType>::getInstance()->getValueConstructorJS();

	if(jsConstructor != nullptr) {
		cbOutput construct(*jsConstructor, &output);

		arg.toJS(construct);
	}

	return(output);
}

template <typename ArgType>
ArgType BindingType<ArgType>::fromWireType(WireType arg) {
	NanScope();

	auto target = arg->ToObject();
	auto fromJS = target->Get(NanNew<v8::String>("fromJS"));

	ArgStorage<ArgType> wrapper;

	cbFunction converter(v8::Handle<v8::Function>::Cast(fromJS));

//	cbFunction *constructor = BindClass<ArgType>::getInstance()->getValueConstructorCC();

	v8::Local<v8::FunctionTemplate> constructorTemplate = NanNew<v8::FunctionTemplate>(
		&BindClass<ArgType>::valueConstructorCaller,
		NanNew<v8::External>(&wrapper)
	);
//	cbFunction constructor(constructorTemplate->GetFunction());
	auto constructor = constructorTemplate->GetFunction();

	converter.callMethod<void>(target, constructor);

//	wrapper.init();

fprintf(stderr, "MOI\n");

	return(wrapper.getBound());
}

} // namespace
