// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#ifdef BUILDING_NODE_EXTENSION

#include <cstring>
#include <unordered_set>

#include "nbind/BindDefiner.h"

using namespace v8;
using namespace nbind;

// Convert getter names like "getFoo" into property names like "foo".
// This could be so much more concisely written with regexps...
const char *stripGetterPrefix(const char *name, char *&nameBuf) {
	if(
		strlen(name) <= 3 ||
		(name[0] != 'G' && name[0] != 'g') ||
		name[1] != 'e' ||
		name[2] != 't'
	) return(name);

	char c = name[3];

	// "Get_foo", "get_foo" => Remove 4 first characters.
	if(c == '_') return(name + 4);

	// "Getfoo", "getfoo" => Remove 3 first characters.
	if(c >= 'a' && c <= 'z') return(name + 3);

	if(c >= 'A' && c <= 'Z') {
		// "GetFOO", "getFOO" => Remove first 3 characters.
		if(name[4] >= 'A' && name[4] <= 'Z') return(name + 3);
	} else return(name);

	// "GetFoo", "getFoo" => Remove 3 first characters,
	// make a modifiable copy and lowercase first letter.

	if(nameBuf != nullptr) free(nameBuf);
	nameBuf = strdup(name + 3);

	if(nameBuf != nullptr) {
		nameBuf[0] = c + ('a' - 'A');
		return(nameBuf);
	}

	// Memory allocation failed.
	// The world will soon end anyway, so just declare
	// the getter without stripping the "get" prefix.

	return(name);
}

NBindID :: NBindID(TYPEID id) : id(id), name(nullptr) {}

NBindID :: NBindID(const NBindID &other) :
	id(other.id), name(other.name ? strdup(other.name) : nullptr) {}

NBindID :: NBindID(NBindID &&other) : id(other.id), name(other.name) {
	other.name = nullptr;
}

NBindID :: ~NBindID() {
	if(name) delete(name);
	name = nullptr;
}

const void *NBindID :: getStructure() const {
	return(structure);
}

StructureType NBindID :: getStructureType() const {
	return(*structureType);
}

const char *NBindID :: toString() {
	if(!name) {
		static const char *alphabet = "0123456789abcdef";

		char *newName = new char[sizeof(id) * 2 + 1];
		unsigned int pos = sizeof(id) * 2;
		uintptr_t code = reinterpret_cast<uintptr_t>(id);

		newName[pos] = 0;

		while(pos--) {
			newName[pos] = alphabet[code & 15];
			code >>= 4;
		}

		name = newName;
	}

	return(name);
}

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
			case SignatureType :: none:

				// Impossible!
				abort();

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

static void initModule(Local<Object> exports) {
	SignatureParam *param;

	for(auto &func : getFunctionList()) {
		const BaseSignature *signature = func.getSignature();

		param = new SignatureParam();
		param->methodNum = func.getNum();

		Local<FunctionTemplate> functionTemplate = Nan::New<FunctionTemplate>(
			reinterpret_cast<BindClassBase::jsMethod *>(signature->getCaller()),
			Nan::New<v8::External>(param)
		);

		Local<v8::Function> jsFunction = Nan::GetFunction(functionTemplate).ToLocalChecked();

		exports->Set(
			Nan::New<String>(func.getName()).ToLocalChecked(),
			jsFunction
		);
	}

	// Base class for all C++ objects.

	Local<FunctionTemplate> superTemplate = Nan::New<FunctionTemplate>(nop);

	// Wrapper for temporary data pointer when passing objects by value.

	auto storageTemplate = Nan::New<ObjectTemplate>();
	storageTemplate->SetInternalFieldCount(1);
	Nan::SetCallAsFunctionHandler(storageTemplate, Overloader::createValue);

	// Create all class constructor templates.

	auto &classList = getClassList();

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

		if(bindClass->isReady()) continue;

		param = new SignatureParam();
		param->overloadNum = bindClass->wrapperConstructorNum;

		auto constructorTemplate = Nan::New<FunctionTemplate>(
			Overloader::create,
			Nan::New<v8::External>(param)
		);

		constructorTemplate->SetClassName(Nan::New<String>(bindClass->getName()).ToLocalChecked());
		constructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);

		bindClass->constructorTemplate.Reset(constructorTemplate);
		bindClass->superTemplate.Reset(superTemplate);
		bindClass->storageTemplate.Reset(storageTemplate);
	}

	// Add NBind reference to base class to enforce its visibility.

	Nan::SetTemplate(
		superTemplate,
		"NBind",
		Nan::New(BindClass<NBind>::getInstance().constructorTemplate)
	);

	// Define inheritance between class constructor templates and add methods.

	for(auto *bindClass : classList) {
		if(bindClass->isReady()) continue;

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

		bindClass->setReady();
	}
}

#include "nbind/nbind.h"

NBIND_CLASS(NBind) {
	construct<>();

	method(bind_value);
	method(reflect);
	method(queryType);
}

NBIND_CLASS(NBindID) {
	method(toString);
}

NODE_MODULE(nbind, initModule)

#endif
