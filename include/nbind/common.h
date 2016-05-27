// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

class NBind {

public:

	static void bind_value(const char *name, cbFunction &func);

};

std::forward_list<BindClassBase *> &getClassList(void);

void registerClass(BindClassBase &bindClass);

} // namespace
