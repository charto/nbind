// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include "nbind/nbind.h"

using namespace nbind;

typedef BaseSignature::Type SigType;

void listMethods(uint64_t classType, std::forward_list<MethodDef> &methodList, cbFunction &outMethod) {
	for(auto &func : methodList) {
		const BaseSignature *signature = func.getSignature();

		if(signature == nullptr) {
			continue;
		}

		auto rawTypes = signature->getTypeList();
		std::vector<uint64_t> argTypes;

		unsigned int arity = signature->getArity() + 1;
		argTypes.reserve(arity);

		for(uint32_t num = 0; num < arity; ++num) {
			argTypes.push_back(reinterpret_cast<uint64_t>(rawTypes[num]));
		}

		outMethod(
			classType,
			func.getName(),
			static_cast<unsigned int>(signature->getType()),
			argTypes
		);
	}
}

void NBind :: reflect(
	cbFunction &outClass,
	cbFunction &outMethod
) {

	for(auto *bindClass : getClassList()) {
		if(bindClass->isDuplicate()) continue;

		const TYPEID *classTypes = bindClass->getTypes();
		uint64_t classType = reinterpret_cast<uint64_t>(classTypes[0]);

		outClass(
			classType,
			reinterpret_cast<uint64_t>(classTypes[1]),
			reinterpret_cast<uint64_t>(classTypes[2]),
			bindClass->getName()
		);

		listMethods(classType, bindClass->getMethodList(), outMethod);
	}

	listMethods(0, getFunctionList(), outMethod);
}
