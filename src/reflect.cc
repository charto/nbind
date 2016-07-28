// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include "nbind/nbind.h"

using namespace nbind;

typedef BaseSignature :: SignatureType SignatureType;

void listMethods(uintptr_t classType, std::forward_list<MethodDef> &methodList, cbFunction &outMethod) {
	for(auto &func : methodList) {
		const BaseSignature *signature = func.getSignature();

		if(signature == nullptr) {
			continue;
		}

		auto rawTypePtr = signature->getTypeList();
		std::vector<uintptr_t> typeIdList;

		unsigned int arity = signature->getArity() + 1;
		typeIdList.reserve(arity);

		while(arity--) {
			typeIdList.push_back(reinterpret_cast<uintptr_t>(*rawTypePtr));
			++rawTypePtr;
		}

		const char **rawPolicyPtr = signature->getPolicies();
		std::vector<const char *> policyNameList;

		while(*rawPolicyPtr) {
			policyNameList.push_back(*rawPolicyPtr);
			++rawPolicyPtr;
		}

		outMethod(
			classType,
			func.getName(),
			static_cast<unsigned int>(signature->getType()),
			typeIdList,
			policyNameList
		);
	}
}

void NBind :: reflect(
	cbFunction &outPrimitive,
	cbFunction &outType,
	cbFunction &outClass,
	cbFunction &outMethod
) {

	const void **primitiveData = getPrimitiveList();
	const uint32_t *sizePtr = static_cast<const uint32_t *>(primitiveData[1]);
	const uint8_t *flagPtr = static_cast<const uint8_t *>(primitiveData[2]);

	for(const TYPEID *type = static_cast<const TYPEID *>(primitiveData[0]); *type; ++type) {
		outPrimitive(
			reinterpret_cast<uintptr_t>(*type),
			*(sizePtr++),
			*(flagPtr++)
		);
	}

	for(const void **type = getNamedTypeList(); *type; type += 2) {
		outType(
			reinterpret_cast<uintptr_t>(type[0]),
			static_cast<const char *>(type[1])
		);
	}

	for(auto *bindClass : getClassList()) {
		if(!bindClass) continue;

		const TYPEID *classTypes = bindClass->getTypes();
		uintptr_t classType = reinterpret_cast<uintptr_t>(classTypes[0]);

		outClass(
			classType,
			reinterpret_cast<uintptr_t>(classTypes[1]),
			reinterpret_cast<uintptr_t>(classTypes[2]),
			bindClass->getName()
		);
	}

	for(auto *bindClass : getClassList()) {
		if(!bindClass) continue;

		uintptr_t classType = reinterpret_cast<uintptr_t>(bindClass->getTypes()[0]);

		listMethods(classType, bindClass->getMethodList(), outMethod);
	}

	listMethods(0, getFunctionList(), outMethod);
}

void NBind :: queryType(
	uintptr_t id,
	cbFunction &outTypeDetail
) {
	VectorStructure *vectorSpec;
	ArrayStructure *arraySpec;

	StructureType placeholderFlag = *reinterpret_cast<const StructureType *>(id);

	switch(placeholderFlag) {
		case StructureType :: vector:
			vectorSpec = reinterpret_cast<VectorStructure *>(id);
			outTypeDetail(
				static_cast<unsigned char>(placeholderFlag),
				reinterpret_cast<uintptr_t>(vectorSpec->member)
			);

			break;

		case StructureType :: array:
			arraySpec = reinterpret_cast<ArrayStructure *>(id);
			outTypeDetail(
				static_cast<unsigned char>(placeholderFlag),
				reinterpret_cast<uintptr_t>(arraySpec->member),
				arraySpec->length
			);

			break;

		default:
			outTypeDetail(static_cast<unsigned char>(placeholderFlag));
	}
}
