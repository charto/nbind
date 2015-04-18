// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#ifdef BUILDING_NODE_EXTENSION

#include <cstring>

#include "nbind/Binding.h"

using namespace v8;
using namespace nbind;

class NBind {};

Persistent<Object> nbind::constructorStore;

// Linkage for module-wide error message.
char *Bindings :: message;

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

	Local<v8::Function> nBindConstructor;

	for(auto *bindClass : getClassList()) {
		if(bindClass->isReady()) continue;

		bindClass->init();

		Local<FunctionTemplate> constructorTemplate = NanNew<FunctionTemplate>(bindClass->createPtr);

		constructorTemplate->SetClassName(NanNew<String>(bindClass->getName()));
		constructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);

		for(auto &method : bindClass->getMethodList()) {
			NanSetPrototypeTemplate(constructorTemplate, method.getName(),
				NanNew<FunctionTemplate>(
					method.getSignature(),
					NanNew<Number>(method.getNum())
				)->GetFunction()
			);
		}

		for(auto &func : bindClass->getFunctionList()) {
			NanSetTemplate(constructorTemplate, func.getName(),
				NanNew<FunctionTemplate>(
					func.getSignature(),
					NanNew<Number>(func.getNum())
				)->GetFunction()
			);
		}

		Local<ObjectTemplate> proto = constructorTemplate->PrototypeTemplate();
		char *nameBuf = nullptr;

		for(auto &access : bindClass->getAccessorList()) {
			proto->SetAccessor(
				NanNew<String>(stripGetterPrefix(access.getName(), nameBuf)),
				access.getGetterSignature(),
				access.getSetterSignature(),
				NanNew<Number>((access.getSetterNum() << accessorSetterShift) | access.getGetterNum())
			);
		}

		// Add NBind references to other classes to enforce visibility.
		if(bindClass == BindClass<NBind>::getInstance()) {
			nBindConstructor = constructorTemplate->GetFunction();
		} else {
			NanSetTemplate(constructorTemplate, "NBind", nBindConstructor);
		}

		exports->Set(
			NanNew<String>(bindClass->getName()),
			constructorTemplate->GetFunction()
		);
	}

	// Keep a persistent table of class constructor functions.
	NanAssignPersistent(constructorStore, exports);
}

#include "nbind/BindingShort.h"

NBIND_CLASS(NBind) {
	construct<>();
}

#endif
