// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include "nbind/nbind.h"

using namespace nbind;

typedef BaseSignature :: SignatureType SignatureType;

void listMethods(NBindType classType, std::forward_list<MethodDef> &methodList, cbFunction &outMethod) {
	for(auto &func : methodList) {
		const BaseSignature *signature = func.getSignature();

		if(signature == nullptr) {
			continue;
		}

		auto rawTypePtr = signature->getTypeList();
		std::vector<NBindType> typeIdList;

		unsigned int arity = signature->getArity() + 1;
		typeIdList.reserve(arity);

		while(arity--) {
			typeIdList.push_back(NBindType(*rawTypePtr));
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
			NBindType(*type),
			*(sizePtr++),
			*(flagPtr++)
		);
	}

	for(const void **type = getNamedTypeList(); *type; type += 2) {
		outType(
			NBindType(type[0]),
			static_cast<const char *>(type[1])
		);
	}

	for(auto *bindClass : getClassList()) {
		if(!bindClass) continue;

		const TYPEID *classTypes = bindClass->getTypes();

		outClass(
			NBindType(classTypes[0]),
			NBindType(classTypes[1]),
			NBindType(classTypes[2]),
			bindClass->getName()
		);
	}

	listMethods(NBindType(nullptr), getFunctionList(), outMethod);

	for(auto *bindClass : getClassList()) {
		if(!bindClass) continue;

		listMethods(
			NBindType(bindClass->getTypes()[0]),
			bindClass->getMethodList(),
			outMethod
		);
	}
}

void NBind :: queryType(
	NBindType id,
	cbFunction &outTypeDetail
) {
	const ParamStructure *paramSpec;
	const ArrayStructure *arraySpec;

	StructureType placeholderFlag = id.getStructureType();

	switch(placeholderFlag) {
		case StructureType :: vector:
			paramSpec = static_cast<const ParamStructure *>(id.getStructure());
			outTypeDetail(
				static_cast<unsigned char>(placeholderFlag),
				NBindType(paramSpec->target)
			);

			break;

		case StructureType :: array:
			arraySpec = static_cast<const ArrayStructure *>(id.getStructure());
			outTypeDetail(
				static_cast<unsigned char>(placeholderFlag),
				NBindType(arraySpec->member),
				arraySpec->length
			);

			break;

		default:
			outTypeDetail(static_cast<unsigned char>(placeholderFlag));
	}
}
