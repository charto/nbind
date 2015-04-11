// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#ifdef BUILDING_NODE_EXTENSION

#include "Binding.h"

using namespace v8;
using namespace nbind;

class NBind {};

Persistent<Object> nbind::constructorStore;

// Linkage for module-wide error message.
char *Bindings :: message;

void Bindings :: registerClass(BindClassBase *bindClass) {
	getClassList().emplace_front(bindClass);
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

#include "BindingShort.h"

NBIND_CLASS(NBind) {
	construct<>();
}

#endif
