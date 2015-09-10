// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

template<typename...> struct TypeList {};

namespace nbind {

class Bindings {

public:

	static void registerClass(BindClassBase &bindClass);
	static void initModule();
//	static void setValueConstructorByName(const char *name, cbFunction &func);

private:

	// Linkage for a list of all C++ class wrappers.

	static std::forward_list<BindClassBase *> &getClassList() {
		static std::forward_list<BindClassBase *> classList;
		return(classList);
	}

};

} // namespace
