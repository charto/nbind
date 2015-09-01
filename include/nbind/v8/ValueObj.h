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

	ArgStorage() : dummy(0) {}

	~ArgStorage() {
		if(isValid) data.~Bound();
		isValid = false;
	}

	template <typename... Args>
	void init(Args&&... args) {
		::new(&data) Bound(std::forward<Args>(args)...);
		isValid = true;
	}

	Bound getBound() {
		return(std::move(data));
	}

private:

	union {
		int dummy;
		Bound data;
	};

	bool isValid = false;

};

template<class Bound, typename ArgList> struct ConstructorInfo;

template<class Bound, typename... Args>
struct ConstructorInfo<Bound, TypeList<Args...>> {

public:

	static const char *getClassName() {return(classNameStore());}
	static void setClassName(const char *className) {classNameStore() = className;}

#ifdef BUILDING_NODE_EXTENSION
	// Make sure prototype matches NanWrapperConstructorTypeBuilder!
	template <typename... NanArgs>
	static BindWrapper<Bound> *makeWrapper(NanArgs... args) noexcept(false) {
		// Note that Args().get may throw.
		return(new BindWrapper<Bound>(Args(std::forward<NanArgs>(args)...).get(args...)...));
	}

	// Make sure prototype matches NanValueConstructorTypeBuilder!
	template <typename... NanArgs>
	static void makeValue(ArgStorage<Bound> &storage, NanArgs... args) {
		storage.init(Args(std::forward<NanArgs>(args)...).get(args...)...);
	}
#endif // BUILDING_NODE_EXTENSION

private:

	static const char *&classNameStore() {
		static const char *className;
		return(className);
	}

};

// Call the toJS method of a returned C++ object, to convert it into a JavaScript object.
// This is used when a C++ function is called from JavaScript.
// A functor capable of calling the correct JavaScript constructor is passed to toJS,
// which must call the functor with arguments in the correct order.
// The functor calls the JavaScript constructor and writes a pointer to the resulting object
// directly into a local handle called "output" which is returned to JavaScript.

template <typename ArgType>
inline WireType BindingType<ArgType *>::toWireType(ArgType *arg) {
	v8::Local<v8::Value> output = Nan::Undefined();

	if(arg != nullptr) {
		cbFunction *jsConstructor = BindClass<ArgType>::getInstance()->getValueConstructorJS();

		if(jsConstructor != nullptr) {
			cbOutput construct(*jsConstructor, &output);

			arg->toJS(construct);
		} else {
			throw(std::runtime_error("Value type JavaScript class is missing or not registered"));
		}
	}

	return(output);
}

template <typename ArgType>
inline WireType BindingType<ArgType>::toWireType(ArgType arg) {
	v8::Local<v8::Value> output = Nan::Undefined();
	cbFunction *jsConstructor = BindClass<ArgType>::getInstance()->getValueConstructorJS();

	if(jsConstructor != nullptr) {
		cbOutput construct(*jsConstructor, &output);

		arg.toJS(construct);
	} else {
		throw(std::runtime_error("Value type JavaScript class is missing or not registered"));
	}

	return(output);
}

template <typename ArgType>
ArgType BindingType<ArgType>::fromWireType(WireType arg) noexcept(false) {
	Nan::HandleScope();

	auto target = arg->ToObject();
	auto fromJS = target->Get(Nan::New<v8::String>("fromJS").ToLocalChecked());

	if(!fromJS->IsFunction()) throw(std::runtime_error("Type mismatch"));

	ArgStorage<ArgType> wrapper;

	cbFunction converter(v8::Local<v8::Function>::Cast(fromJS));

	v8::Local<v8::FunctionTemplate> constructorTemplate = Nan::New<v8::FunctionTemplate>(
		&BindClass<ArgType>::valueConstructorCaller,
		Nan::New<v8::External>(&wrapper)
	);

	auto constructor = constructorTemplate->GetFunction();

	converter.callMethod<void>(target, constructor);

	const char *message = Status::getError();
	if(message) throw(std::runtime_error(message));

	return(wrapper.getBound());
}
} // namespace
