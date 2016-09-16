// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

std::forward_list<BindClassBase *> &getClassList(void);
std::forward_list<MethodDef> &getFunctionList(void);
const void **getPrimitiveList(void);
const void **getNamedTypeList(void);

void registerClass(BindClassBase &bindClass);
void registerFunction(
	const char *name,
	funcPtr ptr,
	unsigned int num,
	BaseSignature *signature,
	TypeFlags flags
);

} // namespace
