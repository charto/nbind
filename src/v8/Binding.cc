// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#ifdef BUILDING_NODE_EXTENSION

#include <cstring>

#include "nbind/BindDefiner.h"

using namespace v8;
using namespace nbind;

class NBind {

public:

	static void bind(const char *name, cbFunction &func) {
		Bindings::setValueConstructorByName(name, func);
	}

};

void Bindings :: setValueConstructorByName(const char *name, cbFunction &func) {
	for(auto *bindClass : getClassList()) {
		if(strcmp(bindClass->getName(), name) == 0) {
			bindClass->setValueConstructorJS(func);
			break;
		}
	}
}

// Linkage for module-wide error message.
const char *Status :: message;

void Bindings :: registerClass(BindClassBase *bindClass) {
	getClassList().emplace_front(bindClass);
}

// Convert getter names like "getFoo" into property names like "foo".
// This could be so much more concisely written with regexps...
const char *stripGetterPrefix(const char *name, char *&nameBuf) {
	if((name[0] == 'G' || name[0] == 'g') && name[1] == 'e' && name[2] == 't') {
		char c = name[3];

		if(c == '_') {
			// "Get_foo", "get_foo" => Remove 4 first characters.

			name += 4;
		} else if(c >= 'a' && c <= 'z') {
			// "Getfoo", "getfoo" => Remove 3 first characters.

			name += 3;
		} else if(c >= 'A' && c <= 'Z') {
			// "GetFoo", "getFoo" => Remove 3 first characters,
			// make a modifiable copy and lowercase first letter.

			if(nameBuf != nullptr) free(nameBuf);
			nameBuf = strdup(name + 3);

			if(nameBuf != nullptr) {
				nameBuf[0] = c + ('a' - 'A');
				name = nameBuf;
			} else {
				// Memory allocation failed.
				// The world will soon end anyway, so just declare
				// the getter without stripping the "get" prefix.
			}
		}
	}

	return(name);
}

void Bindings :: initModule(Handle<Object> exports) {
	// Register NBind a second time to make sure it's first on the list
	// of classes and gets defined first, so pointers to it can be added
	// to other classes to enforce its visibility in npm exports.
	registerClass(BindClass<NBind>::getInstance());

	Local<Function> nBindConstructor;

	for(auto *bindClass : getClassList()) {
		// Avoid registering the same class twice.
		if(bindClass->isReady()) continue;

		bindClass->init();

		Local<FunctionTemplate> constructorTemplate = Nan::New<FunctionTemplate>(bindClass->createPtr);

		constructorTemplate->SetClassName(Nan::New<String>(bindClass->getName()).ToLocalChecked());
		constructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);

		Local<ObjectTemplate> proto = constructorTemplate->PrototypeTemplate();
		char *nameBuf = nullptr;

		funcPtr setter = nullptr;
		unsigned int setterNum = 0;

		for(auto &func : bindClass->getMethodList()) {
			// TODO: Support for function overloading goes here.

			switch(func.getType()) {
				case MethodDef::Type::method:
					Nan::SetPrototypeTemplate(constructorTemplate, func.getName(),
						Nan::New<FunctionTemplate>(
							reinterpret_cast<BindClassBase::jsMethod *>(func.getCaller()),
							Nan::New<Number>(func.getNum())
						)->GetFunction()
					);

					break;

				case MethodDef::Type::function:
					Nan::SetTemplate(constructorTemplate, func.getName(),
						Nan::New<FunctionTemplate>(
							reinterpret_cast<BindClassBase::jsMethod *>(func.getCaller()),
							Nan::New<Number>(func.getNum())
						)->GetFunction()
					);

					break;

				case MethodDef::Type::setter:
					setter = func.getCaller();
					setterNum = func.getNum();

					break;

				case MethodDef::Type::getter:
					Nan::SetAccessor(
						proto,
						Nan::New<String>(stripGetterPrefix(func.getName(), nameBuf)).ToLocalChecked(),
						reinterpret_cast<BindClassBase::jsGetter *>(func.getCaller()),
						reinterpret_cast<BindClassBase::jsSetter *>(setter),
						Nan::New<Number>((setterNum << accessorSetterShift) | func.getNum())
					);

					break;
			}
		}

		if(nameBuf != nullptr) free(nameBuf);

		// Add NBind references to other classes to enforce visibility.
		if(bindClass == BindClass<NBind>::getInstance()) {
			nBindConstructor = constructorTemplate->GetFunction();
		} else {
			Nan::SetTemplate(constructorTemplate, "NBind", nBindConstructor);
		}

		Local<v8::Function> jsConstructor = constructorTemplate->GetFunction();

		bindClass->setConstructorHandle(jsConstructor);

		exports->Set(
			Nan::New<String>(bindClass->getName()).ToLocalChecked(),
			jsConstructor
		);
	}
}

#include "nbind/BindingShort.h"

NBIND_CLASS(NBind) {
	construct<>();

	method(bind);
}

NODE_MODULE(nbind, nbind::Bindings::initModule)

#endif
