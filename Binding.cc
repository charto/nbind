// This file is part of nbind, copyright (C) 2014 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#ifdef BUILDING_NODE_EXTENSION

#include "Binding.h"

using namespace v8;
using namespace nbind;

namespace nbind {
	Persistent<Object> constructorStore;
}

void Bindings :: registerClass(BindClassBase *bindClass) {
	getClassList().emplace_front(bindClass);
}

void Bindings :: initModule(Handle<Object> exports) {
	for(auto *bindClass : getClassList()) {
		Local<FunctionTemplate> constructorTemplate = NanNew<FunctionTemplate>(bindClass->createPtr);

		constructorTemplate->SetClassName(NanNew<String>(bindClass->getName()));
		constructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);

		for(auto &method : bindClass->getMethodList()) {
			NanSetPrototypeTemplate(constructorTemplate, method.getName(),
				NanNew<FunctionTemplate>(method.getMethod())->GetFunction());
		}

		const auto &constructor = constructorTemplate->GetFunction();

		exports->Set(NanNew<String>(bindClass->getName()), constructor);
	}

	// Keep a persistent table of class constructor functions.
	NanAssignPersistent(constructorStore, exports);
}

#endif
