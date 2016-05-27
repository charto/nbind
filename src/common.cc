// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
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

// Linkage for a list of all C++ class wrappers.

std::forward_list<BindClassBase *> &nbind :: getClassList() {
	static std::forward_list<BindClassBase *> classList;
	return(classList);
}

void nbind :: registerClass(BindClassBase &bindClass) {
	getClassList().emplace_front(&bindClass);
}
