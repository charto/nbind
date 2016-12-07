// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#if defined(BUILDING_NODE_EXTENSION) && !defined(NODE_USE_NAPI)

#include <cstring>
#include <unordered_set>

#include "nbind/BindDefiner.h"

using namespace v8;
using namespace nbind;

typedef BaseSignature :: SignatureType SignatureType;

static void registerMethods(
	BindClassBase &bindClass,
	Local<FunctionTemplate> constructorTemplate,
	bool staticOnly
) {
	Local<ObjectTemplate> proto = constructorTemplate->PrototypeTemplate();
	char *nameBuf = nullptr;

	funcPtr setter = nullptr;
	funcPtr getter = nullptr;
	// unsigned int getterNum = 0; unused for now.
	unsigned int setterNum = 0;
	SignatureParam *param;

	for(auto &func : bindClass.getMethodList()) {
		// TODO: Support for function overloading goes here.

		const BaseSignature *signature = func.getSignature();

		if(signature == nullptr) {

			/*
			Currently properties must have getters so this is unused.
			if(func.getName() == emptyGetter) {
				getter = nullptr;
				getterNum = 0;
			}
			*/

			if(func.getName() == emptySetter) {
				setter = nullptr;
				setterNum = 0;
			}

			continue;
		}

		if(staticOnly && signature->getType() != SignatureType :: func) continue;

		param = new SignatureParam();

		switch(signature->getType()) {
			case SignatureType :: method:
				param->methodNum = func.getNum();
				Nan::SetPrototypeTemplate(constructorTemplate, func.getName(),
					Nan::New<FunctionTemplate>(
						reinterpret_cast<BindClassBase::jsMethod *>(signature->getCaller()),
						Nan::New<v8::External>(param)
					)
				);

				break;

			case SignatureType :: func:
				param->methodNum = func.getNum();
				Nan::SetTemplate(constructorTemplate, func.getName(),
					Nan::New<FunctionTemplate>(
						reinterpret_cast<BindClassBase::jsMethod *>(signature->getCaller()),
						Nan::New<v8::External>(param)
					)
				);

				break;

			case SignatureType :: setter:
				setter = signature->getCaller();
				setterNum = func.getNum();

				break;

			case SignatureType :: getter:
				getter = signature->getCaller();

				param->setterNum = setterNum;
				param->methodNum = func.getNum();
				Nan::SetAccessor(
					proto,
					Nan::New<String>(stripGetterPrefix(func.getName(), nameBuf)).ToLocalChecked(),
					reinterpret_cast<BindClassBase::jsGetter *>(getter),
					reinterpret_cast<BindClassBase::jsSetter *>(setter),
					Nan::New<v8::External>(param)
				);

				break;

			case SignatureType :: construct:

				// Constructors in method list are ignored.
				// They're handled by overloaders for wrappers and values.

				break;
		}
	}

	if(nameBuf != nullptr) free(nameBuf);
}

/** Register class members and simulate multiple inheritance. */

static void registerSuperMethods(
	BindClassBase &bindClass,
	/** Index of first superclass not natively inherited,
	  * or -1 to recursively ignore all members. */
	signed int firstSuper,
	Local<FunctionTemplate> constructorTemplate,
	std::unordered_set<BindClassBase *> &visitTbl
) {
	// Stop if this class has already been visited.
	if(visitTbl.find(&bindClass) != visitTbl.end()) return;

	// Mark this class visited.
	visitTbl.insert(&bindClass);

	signed int superNum = 0;
	signed int nextFirst;

	for(auto &spec : bindClass.getSuperClassList()) {
		if(superNum++ < firstSuper || firstSuper < 0) {
			// Non-static contents of the initial first superclass and all of its
			// superclasses have already been inherited through the prototype
			// chain. Mark them visited recursively and inherit static methods.
			nextFirst = -1;
		} else {
			// Complete contents of all other superclasses must be included
			// recursively.
			nextFirst = 0;
		}

		registerSuperMethods(spec.superClass, nextFirst, constructorTemplate, visitTbl);
	}

	// Include (possibly only static) contents in class constructor template.
	registerMethods(bindClass, constructorTemplate, firstSuper < 0);
}

static void nop(const Nan::FunctionCallbackInfo<v8::Value> &args) {
	args.GetReturnValue().Set(Nan::Undefined());
}

static void initModule(Handle<Object> exports) {
	SignatureParam *param;

	for(auto &func : getFunctionList()) {
		const BaseSignature *signature = func.getSignature();

		param = new SignatureParam();
		param->methodNum = func.getNum();

		Local<FunctionTemplate> functionTemplate = Nan::New<FunctionTemplate>(
			reinterpret_cast<BindClassBase::jsMethod *>(signature->getCaller()),
			Nan::New<v8::External>(param)
		);

		Local<v8::Function> jsFunction = functionTemplate->GetFunction();

		exports->Set(
			Nan::New<String>(func.getName()).ToLocalChecked(),
			jsFunction
		);
	}

	// Base class for all C++ objects.

	Local<FunctionTemplate> superTemplate = Nan::New<FunctionTemplate>(nop);

	auto &classList = getClassList();

	// Create all class constructor templates.

	for(auto *bindClass : classList) bindClass->unvisit();

	auto posPrev = classList.before_begin();
	auto pos = classList.begin();

	while(pos != classList.end()) {
		auto *bindClass = *pos++;

		// Avoid registering the same class twice.
		if(bindClass->isVisited()) {
			classList.erase_after(posPrev);
			continue;
		}

		bindClass->visit();
		++posPrev;

		param = new SignatureParam();
		param->overloadNum = bindClass->wrapperConstructorNum;

		Local<FunctionTemplate> constructorTemplate = Nan::New<FunctionTemplate>(
			Overloader::create,
			Nan::New<v8::External>(param)
		);

		constructorTemplate->SetClassName(Nan::New<String>(bindClass->getName()).ToLocalChecked());
		constructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);

		bindClass->constructorTemplate.Reset(constructorTemplate);
		bindClass->superTemplate.Reset(superTemplate);
	}

	// Add NBind reference to base class to enforce its visibility.

	Nan::SetTemplate(
		superTemplate,
		"NBind",
		Nan::New(BindClass<NBind>::getInstance().constructorTemplate)
	);

	// Define inheritance between class constructor templates and add methods.

	for(auto *bindClass : classList) {
		Local<FunctionTemplate> constructorTemplate = Nan::New(bindClass->constructorTemplate);

		auto superClassList = bindClass->getSuperClassList();

		if(!superClassList.empty()) {
			// This fails if the constructor template of any other child class
			// has already been instantiated!
			// TODO: Is this correct? What the superclass inherits may still
			// be undefined. Should this be in registerSuperMethods instead?
			constructorTemplate->Inherit(Nan::New(superClassList.front().superClass.constructorTemplate));
		} else {
			constructorTemplate->Inherit(superTemplate);
		}

		// Add methods of the class and all its superclasses not inherited
		// through the prototype chain.

		std::unordered_set<BindClassBase *> visitTbl;
		registerSuperMethods(*bindClass, 1, constructorTemplate, visitTbl);

		Nan::SetPrototypeTemplate(constructorTemplate, "free",
			Nan::New<FunctionTemplate>(
				bindClass->getDeleter()
			)
		);
	}

	// Instantiate and export class constructor templates.

	for(auto *bindClass : classList) {
		// Instantiate the constructor template.
		Local<Function> jsConstructor = Nan::GetFunction(Nan::New(bindClass->constructorTemplate)).ToLocalChecked();

		Overloader::setConstructorJS(bindClass->wrapperConstructorNum, jsConstructor);
		Overloader::setPtrWrapper(bindClass->wrapperConstructorNum, bindClass->wrapPtr);

		exports->Set(
			Nan::New<String>(bindClass->getName()).ToLocalChecked(),
			jsConstructor
		);
	}
}

NODE_MODULE(nbind, initModule)

#endif
