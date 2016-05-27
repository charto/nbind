// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

std::forward_list<BindClassBase *> &getClassList(void);

void registerClass(BindClassBase &bindClass);

} // namespace
