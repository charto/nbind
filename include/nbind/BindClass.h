// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Templated singleton class for each C++ class accessible from Node.js.
// Stores their information defined in static constructors, until the Node.js
// plugin is initialized.

class BindClassBase {

protected:

	// Get type of method definitions to use in function pointers.

	inline static NAN_METHOD(dummyMethod) {NanReturnNull();}
	typedef decltype(dummyMethod) jsMethod;

public:

	class MethodDef {

	public:

		MethodDef(const char *name, unsigned int num, jsMethod *signature) :
			name(name), num(num), signature(signature) {}

		const char *getName() {return(name);}
		unsigned int getNum() {return(num);}
		jsMethod *getSignature() {return(signature);}

	private:

		const char *name;
		unsigned int num;
		jsMethod *signature;

	};

	BindClassBase() {}

	const char *getName() {return(name);}
	void setName(const char *name) {this->name = name;}

	bool isReady() {return(ready);}

	void init() {ready = 1;}

	void addMethod(const char *name, unsigned int num, jsMethod *signature) {
		methodList.emplace_front(name, num, signature);
	}

	void addFunction(const char *name, unsigned int num, jsMethod *signature) {
		funcList.emplace_front(name, num, signature);
	}

	std::forward_list<MethodDef> &getMethodList() {return(methodList);}

	std::forward_list<MethodDef> &getFunctionList() {return(funcList);}

	jsMethod *createPtr;

protected:

	bool ready = 0;
	const char *name;
	std::forward_list<MethodDef> methodList;
	std::forward_list<MethodDef> funcList;

};

template <class Bound> class BindClass : public BindClassBase {

public:

	BindClass() : BindClassBase() {
		createPtr = create;
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
		static std::vector<jsConstructor *> &constructorVect = constructorVectStore();
		signed int oldArity = getArity();

		if(signed(arity) > oldArity) {
			constructorVect.resize(arity + 1);
			for(unsigned int pos = oldArity + 1; pos < arity; pos++) {
				constructorVect[pos] = nullptr;
			}
		}

		constructorVect[arity] = ptr;
	}

	// Get maximum arity among overloaded constructors.
	// Can be -1 if there are no constructors.

	static signed int getArity() {
		return(constructorVectStore().size() - 1);
	}

	// Get constructor by arity.
	// When called, the constructor returns an ObjectWrap.

	static jsConstructor *getConstructorWrapper(unsigned int arity) {
		// Check if constructor was called with more than the maximum number
		// of arguments it can accept.
		if(signed(arity) > getArity()) return(nullptr);

		return(constructorVectStore()[arity]);
	}

	static NAN_METHOD(create);

	// Use a static variable inside a static method to provide linkage for a
	// singleton instance of this class.

	static BindClass *getInstance() {return(instanceStore());}
	void setInstance(BindClass *instance) {instanceStore() = instance;}

	static BindClass *&instanceStore() {
		static BindClass *instance;
		return(instance);
	}

	// Linkage for a table of overloaded constructors
	// (overloads must have different arities).

	static std::vector<jsConstructor *> &constructorVectStore() {
		static std::vector<jsConstructor *> constructorVect;
		return(constructorVect);
	}

};

} // namespace
