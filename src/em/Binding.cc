// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#ifdef EMSCRIPTEN

#include <cstdlib>
#include <cstring>

#include "nbind/BindDefiner.h"

using namespace nbind;

extern "C" {
	extern void _nbind_register_type(TYPEID id, const char *name);
	extern void _nbind_register_types(void **data);
	extern void _nbind_register_class(TYPEID type, const char *name);
	extern void _nbind_register_constructor(TYPEID classType, funcPtr func, const TYPEID *types, unsigned int typeCount);
	extern void _nbind_register_function(TYPEID classType, funcPtr func, const char *name, const TYPEID *types, unsigned int typeCount);
	extern void _nbind_register_method(TYPEID classType, funcPtr func, unsigned int num, const char *name, const TYPEID *types, unsigned int typeCount);
	void nbind_init();
}

const char *nbind :: emptyGetter = "";
const char *nbind :: emptySetter = "";

// Linkage for module-wide error message.
const char *Status :: message;

void Bindings :: registerClass(BindClassBase *bindClass) {
	getClassList().emplace_front(bindClass);
}

typedef BaseSignature::Type SigType;

void Bindings :: initModule() {
	_nbind_register_type(makeTypeID<void>(), "void");
	_nbind_register_type(makeTypeID<bool>(), "bool");

	_nbind_register_types(defineTypes<
		unsigned char,  signed char,    char,
		unsigned short, signed short,
		unsigned int,   signed int,
		unsigned long,  signed long,

		float, double,

		unsigned char *, const unsigned char *,
		signed   char *, const signed   char *,
		         char *, const          char *
	>());

//	_nbind_register_type("cbOutput", listTypes<cbOutput>());
//	_nbind_register_type("cbFunction", listTypes<cbFunction>());

	// Register all classes before any functions, so they'll have type information ready.

	for(auto *bindClass : getClassList()) {
		// Avoid registering the same class twice.
		if(bindClass->isReady()) {
			bindClass->setDuplicate();
			continue;
		}

		bindClass->init();

		_nbind_register_class(bindClass->getTypeID(), bindClass->getName());
	}

	// Register all functions.

	for(auto *bindClass : getClassList()) {
		if(bindClass->isDuplicate()) continue;

		for(auto &func : bindClass->getMethodList()) {

			BaseSignature *signature = func.getSignature();

			if(signature == nullptr) {
				continue;
			}

			switch(signature->getType()) {
				case SigType::method:
					_nbind_register_method(bindClass->getTypeID(), signature->getCaller(), func.getNum(), func.getName(), signature->getTypeList(), signature->getArity() + 1);

					break;

				case SigType::function:
					_nbind_register_function(bindClass->getTypeID(), func.getPtr(), func.getName(), signature->getTypeList(), signature->getArity() + 1);

					break;
			}
		}
	}
}

void nbind_init(void) {
	Bindings::initModule();
}

#endif
