// This file is part of nbind, copyright (C) 2014 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#ifdef BUILDING_NODE_EXTENSION

#include <node.h>
#include <v8.h>

#include "Binding.h"

using namespace v8;
using namespace nbind;

void Bindings::registerClass(BindClassBase *bindClass) {
	getClassList().emplace_front(bindClass);
}

void Bindings::initModule(Handle<Object> exports) {
	for(auto *bindClass : getClassList()) {
		Local<FunctionTemplate> constructorTemplate=FunctionTemplate::New(bindClass->createPtr);

		constructorTemplate->SetClassName(String::NewSymbol(bindClass->getName()));
		constructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);

		for(auto &method : bindClass->getMethodList()) {
			constructorTemplate->PrototypeTemplate()->Set(String::NewSymbol(method.getName()),
				FunctionTemplate::New(method.getMethod())->GetFunction());
		}

		auto &constructor=bindClass->getConstructorPtr();
		constructor=Persistent<Function>::New(constructorTemplate->GetFunction());
		exports->Set(String::NewSymbol(bindClass->getName()),constructor);
	}
}

#endif
