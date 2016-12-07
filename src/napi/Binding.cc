// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#if defined(BUILDING_NODE_EXTENSION) && defined(NODE_USE_NAPI)

//#include <cstring>
//#include <unordered_set>
#include <node_jsvmapi.h>

#include "nbind/BindDefiner.h"

using namespace nbind;

typedef BaseSignature :: SignatureType SignatureType;

/*
void RunCallback(napi_env env, const napi_func_cb_info info) {
  napi_value args[1];
  napi_get_cb_args(env, info, args, 1);
  napi_value cb = args[0];

  napi_value argv[1];
  argv[0] = napi_create_string(env, "hello world");
  napi_call_function(env, napi_get_global_scope(env) , cb, 1, argv);
}
*/

static void registerMethods(
	BindClassBase &bindClass,
	napi_env &env,
	napi_value &obj,
	bool staticOnly
) {
/*
	Local<ObjectTemplate> proto = constructorTemplate->PrototypeTemplate();
	char *nameBuf = nullptr;

	funcPtr setter = nullptr;
	funcPtr getter = nullptr;
	// unsigned int getterNum = 0; unused for now.
	unsigned int setterNum = 0;
	SignatureParam *param;
*/
	napi_value caller;

	for(auto &func : bindClass.getMethodList()) {
		const BaseSignature *signature = func.getSignature();

		if(signature == nullptr) {
			if(func.getName() == emptySetter) {
//				setter = nullptr;
//				setterNum = 0;
			}

			continue;
		}

		if(staticOnly && signature->getType() != SignatureType :: func) continue;

//		param = new SignatureParam();

		switch(signature->getType()) {
			case SignatureType :: method:
/*				param->methodNum = func.getNum();
				Nan::SetPrototypeTemplate(constructorTemplate, func.getName(),
					Nan::New<FunctionTemplate>(
						reinterpret_cast<BindClassBase::jsMethod *>(signature->getCaller()),
						Nan::New<v8::External>(param)
					)
				);
*/
				break;

			case SignatureType :: func:
				caller = napi_create_function(
					env,
					reinterpret_cast<BindClassBase::jsMethod *>(signature->getCaller())
				);

				napi_set_property(
					env,
					caller,
					napi_property_name(env, "_nbindMethodNum"),
					napi_create_number(env, func.getNum())
				);

				napi_set_property(
					env,
					obj,
					napi_property_name(env, func.getName()),
					caller
				);

				break;

			case SignatureType :: setter:
//				setter = signature->getCaller();
//				setterNum = func.getNum();

				break;

			case SignatureType :: getter:
/*				getter = signature->getCaller();

				param->setterNum = setterNum;
				param->methodNum = func.getNum();
				Nan::SetAccessor(
					proto,
					Nan::New<String>(stripGetterPrefix(func.getName(), nameBuf)).ToLocalChecked(),
					reinterpret_cast<BindClassBase::jsGetter *>(getter),
					reinterpret_cast<BindClassBase::jsSetter *>(setter),
					Nan::New<v8::External>(param)
				);
*/
				break;

			case SignatureType :: construct:

				// Constructors in method list are ignored.
				// They're handled by overloaders for wrappers and values.

				break;
		}
	}

//	if(nameBuf != nullptr) free(nameBuf);
}

void initModule(napi_env env, napi_value exports, napi_value module) {
	for(auto &func : getFunctionList()) {
		const BaseSignature *signature = func.getSignature();

		napi_value caller = napi_create_function(
			env,
			reinterpret_cast<BindClassBase::jsMethod *>(signature->getCaller())
		);

		napi_set_property(
			env,
			caller,
			napi_property_name(env, "_nbindMethodNum"),
			napi_create_number(env, func.getNum())
		);

		napi_set_property(
			env,
			exports,
			napi_property_name(env, func.getName()),
			caller
		);
	}

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

		napi_value constructor = napi_create_function(
			env,
			Overloader::create
		);

		napi_set_property(
			env,
			constructor,
			napi_property_name(env, "_nbindOverloadNum"),
			napi_create_number(env, bindClass->wrapperConstructorNum)
		);

		registerMethods(*bindClass, env, constructor, false);

		napi_set_property(
			env,
			exports,
			napi_property_name(env, bindClass->getName()),
			constructor
		);
	}

	for(auto *bindClass : classList) {
	}
}

NODE_MODULE_ABI(nbind, initModule)

#endif
