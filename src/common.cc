// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include "nbind/BindDefiner.h"

using namespace nbind;

const char *nbind :: emptyGetter = "";
const char *nbind :: emptySetter = "";

// Linkage for module-wide error message.
const char *Status :: message;

void NBind :: bind_value(const char *name, cbFunction &func) {
	for(auto *bindClass : getClassList()) {
		if(strcmp(bindClass->getName(), name) == 0) {
			bindClass->setValueConstructorJS(func);
			break;
		}
	}
}

// Linkage for lists of all C++ class and function wrappers.

std::forward_list<BindClassBase *> &nbind :: getClassList() {
	// This stops working if moved outside the function.
	static std::forward_list<BindClassBase *> classList;

	return(classList);
}

std::forward_list<MethodDef> &nbind :: getFunctionList() {
	// This stops working if moved outside the function.
	static std::forward_list<MethodDef> functionList;

	return(functionList);
}

void nbind :: registerClass(BindClassBase &spec) {
	getClassList().emplace_front(&spec);
}

void nbind :: registerFunction(
	const char *name,
	funcPtr ptr,
	unsigned int num,
	BaseSignature *signature
) {
	getFunctionList().emplace_front(name, ptr, num, signature);
}
