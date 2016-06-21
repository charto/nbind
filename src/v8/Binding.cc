// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#ifdef BUILDING_NODE_EXTENSION

#include <cstring>

#include "nbind/BindDefiner.h"

using namespace v8;
using namespace nbind;

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

typedef BaseSignature::Type SigType;

static void initModule(Handle<Object> exports) {
	// Register NBind a second time to make sure it's first on the list
	// of classes and gets defined first, so pointers to it can be added
	// to other classes to enforce its visibility in npm exports.
	registerClass(BindClass<NBind>::getInstance());

	Local<FunctionTemplate> nBindTemplate;

	for(auto &func : getFunctionList()) {
		const BaseSignature *signature = func.getSignature();

		Local<FunctionTemplate> functionTemplate = Nan::New<FunctionTemplate>(
			reinterpret_cast<BindClassBase::jsMethod *>(signature->getCaller()),
			Nan::New<Number>(func.getNum())
		);

		Local<v8::Function> jsFunction = functionTemplate->GetFunction();

		exports->Set(
			Nan::New<String>(func.getName()).ToLocalChecked(),
			jsFunction
		);
	}

	for(auto *bindClass : getClassList()) {
		// Avoid registering the same class twice.
		if(bindClass->isReady()) {
			bindClass->setDuplicate();
			continue;
		}

		bindClass->init();

		Local<FunctionTemplate> constructorTemplate = Nan::New<FunctionTemplate>(
			Overloader::create,
			Nan::New<Number>(bindClass->wrapperConstructorNum << overloadShift)
		);

		constructorTemplate->SetClassName(Nan::New<String>(bindClass->getName()).ToLocalChecked());
		constructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);

		Nan::SetPrototypeTemplate(constructorTemplate, "free",
			Nan::New<FunctionTemplate>(
				bindClass->getDeleter()
			)
		);

		Local<ObjectTemplate> proto = constructorTemplate->PrototypeTemplate();
		char *nameBuf = nullptr;

		funcPtr setter = nullptr;
		funcPtr getter = nullptr;
		unsigned int getterNum = 0;
		unsigned int setterNum = 0;

		for(auto &func : bindClass->getMethodList()) {
			// TODO: Support for function overloading goes here.

			const BaseSignature *signature = func.getSignature();

			if(signature == nullptr) {

				if(func.getName() == emptyGetter) {
					getter = nullptr;
					getterNum = 0;
				}

				if(func.getName() == emptySetter) {
					setter = nullptr;
					setterNum = 0;
				}

				continue;
			}

			switch(signature->getType()) {
				case SigType::method:
					Nan::SetPrototypeTemplate(constructorTemplate, func.getName(),
						Nan::New<FunctionTemplate>(
							reinterpret_cast<BindClassBase::jsMethod *>(signature->getCaller()),
							Nan::New<Number>(func.getNum())
						)
					);

					break;

				case SigType::function:
					Nan::SetTemplate(constructorTemplate, func.getName(),
						Nan::New<FunctionTemplate>(
							reinterpret_cast<BindClassBase::jsMethod *>(signature->getCaller()),
							Nan::New<Number>(func.getNum())
						)
					);

					break;

				case SigType::setter:
					setter = signature->getCaller();
					setterNum = func.getNum();

					break;

				case SigType::getter:
					getter = signature->getCaller();
					getterNum = func.getNum();

					Nan::SetAccessor(
						proto,
						Nan::New<String>(stripGetterPrefix(func.getName(), nameBuf)).ToLocalChecked(),
						reinterpret_cast<BindClassBase::jsGetter *>(getter),
						reinterpret_cast<BindClassBase::jsSetter *>(setter),
						Nan::New<Number>((setterNum << accessorSetterShift) | getterNum)
					);

					break;

				case SigType::constructor:

					// Impossible! Constructors are not added to the method list.
					// They're handled by overloaders for wrappers and values.

					break;
			}
		}

		if(nameBuf != nullptr) free(nameBuf);

		// Add NBind references to other classes to enforce visibility.
		if(bindClass == &BindClass<NBind>::getInstance()) {
			nBindTemplate = constructorTemplate;
		} else {
			Nan::SetTemplate(constructorTemplate, "NBind", nBindTemplate);
		}

		Local<v8::Function> jsConstructor = constructorTemplate->GetFunction();

		Overloader::setConstructorJS(bindClass->wrapperConstructorNum, jsConstructor);

		exports->Set(
			Nan::New<String>(bindClass->getName()).ToLocalChecked(),
			jsConstructor
		);
	}
}

#include "nbind/nbind.h"

NBIND_CLASS(NBind) {
	construct<>();

	method(bind_value);
	method(reflect);
}

NODE_MODULE(nbind, initModule)

#endif
