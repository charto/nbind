// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#ifdef EMSCRIPTEN

#include <cstdlib>
#include <cstring>

#include "nbind/BindDefiner.h"

//using namespace v8;
using namespace nbind;

extern "C" {
	extern void _nbind_register_class(const char *msg);
	extern void _nbind_register_function(const char *msg);
	extern void _nbind_register_method(const char *msg);
}

/*
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
*/

// Linkage for module-wide error message.
const char *Status :: message;

void Bindings :: registerClass(BindClassBase *bindClass) {
	getClassList().emplace_front(bindClass);
}

/*
void Bindings :: initModule(Handle<Object> exports) {
	// Register NBind a second time to make sure it's first on the list
	// of classes and gets defined first, so pointers to it can be added
	// to other classes to enforce its visibility in npm exports.
	registerClass(BindClass<NBind>::getInstance());

	for(auto *bindClass : getClassList()) {
		for(auto &method : bindClass->getMethodList()) {
			NanSetPrototypeTemplate(constructorTemplate, method.getName(),
				NanNew<FunctionTemplate>(
					reinterpret_cast<BindClassBase::jsMethod *>(method.getSignature()),
					NanNew<Number>(method.getNum())
				)->GetFunction()
			);
		}

		for(auto &func : bindClass->getFunctionList()) {
			NanSetTemplate(constructorTemplate, func.getName(),
				NanNew<FunctionTemplate>(
					reinterpret_cast<BindClassBase::jsMethod *>(func.getSignature()),
					NanNew<Number>(func.getNum())
				)->GetFunction()
			);
		}

		Local<ObjectTemplate> proto = constructorTemplate->PrototypeTemplate();
		char *nameBuf = nullptr;

		for(auto &access : bindClass->getAccessorList()) {
			proto->SetAccessor(
				NanNew<String>(stripGetterPrefix(access.getName(), nameBuf)),
				reinterpret_cast<BindClassBase::jsGetter *>(access.getGetterSignature()),
				reinterpret_cast<BindClassBase::jsSetter *>(access.getSetterSignature()),
				NanNew<Number>((access.getSetterNum() << accessorSetterShift) | access.getGetterNum())
			);
		}

		// Add NBind references to other classes to enforce visibility.
		if(bindClass == BindClass<NBind>::getInstance()) {
			nBindConstructor = constructorTemplate->GetFunction();
		} else {
			NanSetTemplate(constructorTemplate, "NBind", nBindConstructor);
		}
	}
}

#include "nbind/BindingShort.h"

NBIND_CLASS(NBind) {
	construct<>();

	method(bind);
}

NODE_MODULE(nbind, nbind::Bindings::initModule)
*/

void Bindings :: initModule() {
	for(auto *bindClass : getClassList()) {
		// Avoid registering the same class twice.
		if(bindClass->isReady()) continue;

		bindClass->init();

		_nbind_register_class(bindClass->getName());

		for(auto &func : bindClass->getMethodList()) {
			switch(func.getType()) {
				case BindClassBase::MethodDef::Type::method:
					_nbind_register_method(func.getName());

					break;

				case BindClassBase::MethodDef::Type::function:
					_nbind_register_function(func.getName());

					break;
			}
		}
	}
}

int main(void) {
	Bindings::initModule();

	return(EXIT_SUCCESS);
}

#endif
