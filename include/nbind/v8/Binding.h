// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Storage for class bindings, populated by BindDefiners created in
// constructors of static BindInvoker objects.

class Bindings {

public:

	static void registerClass(BindClassBase *bindClass);
	static void initModule(v8::Handle<v8::Object> exports);
	static void setValueConstructorByName(const char *name, cbFunction &func);

private:

	// Linkage for a list of all C++ class wrappers.

	static std::forward_list<BindClassBase *> &getClassList() {
		static std::forward_list<BindClassBase *> classList;
		return(classList);
	}

};

extern v8::Persistent<v8::Object> constructorStore;

} // namespace
