// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include "nbind/nbind.h"

using namespace nbind;

typedef BaseSignature :: SignatureType SignatureType;

void listMethods(NBindID classType, std::forward_list<MethodDef> &methodList, cbFunction &outMethod) {
	for(auto &func : methodList) {
		const BaseSignature *signature = func.getSignature();

		if(signature == nullptr) {
			continue;
		}

		auto rawTypePtr = signature->getTypeList();
		std::vector<NBindID> typeIdList;

		unsigned int arity = signature->getArity() + 1;
		typeIdList.reserve(arity);

		while(arity--) {
			typeIdList.push_back(NBindID(*rawTypePtr));
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
	cbFunction &outSuper,
	cbFunction &outMethod
) {

	const void **primitiveData = getPrimitiveList();
	const uint8_t *sizePtr = static_cast<const uint8_t *>(primitiveData[1]);
	const uint8_t *flagPtr = static_cast<const uint8_t *>(primitiveData[2]);

	for(const TYPEID *type = static_cast<const TYPEID *>(primitiveData[0]); *type; ++type) {
		outPrimitive(
			NBindID(*type),
			*(sizePtr++),
			*(flagPtr++)
		);
	}

	for(const void **type = getNamedTypeList(); *type; type += 2) {
		outType(
			NBindID(type[0]),
			static_cast<const char *>(type[1])
		);
	}

	for(BindClassBase *bindClass : getClassList()) {
		NBindID classType{bindClass->getTypes()[0]};

		outClass(
			classType,
			bindClass->getName()
		);
	}

	listMethods(NBindID(nullptr), getFunctionList(), outMethod);

	for(BindClassBase *bindClass : getClassList()) {
		NBindID classType{bindClass->getTypes()[0]};
		auto superClassList = bindClass->getSuperClassList();

		if(!superClassList.empty()) {
			std::vector<NBindID> superIdList;

			for(auto &spec : superClassList) {
				superIdList.push_back(NBindID(spec.superClass.getTypes()[0]));
			}

			outSuper(classType, superIdList);
		}

		listMethods(
			classType,
			bindClass->getMethodList(),
			outMethod
		);
	}
}

External NBind :: queryType(
	NBindID id,
	cbFunction &outTypeDetail
) {
	const ParamStructure *paramSpec;
	const ArrayStructure *arraySpec;
	const CallbackStructure<1> *callbackSpec;
	External result;

	StructureType placeholderFlag = id.getStructureType();
	std::vector<NBindID> typeIdList;
	const TYPEID *rawTypePtr;
	unsigned int arity;

	switch(placeholderFlag) {
		case StructureType :: none:
			result = outTypeDetail.call<External>(
				static_cast<unsigned char>(placeholderFlag)
			);

			break;

		case StructureType :: array:
			arraySpec = static_cast<const ArrayStructure *>(id.getStructure());

			result = outTypeDetail.call<External>(
				static_cast<unsigned char>(placeholderFlag),
				NBindID(arraySpec->member),
				arraySpec->length
			);

			break;

		case StructureType :: callback:
			callbackSpec = static_cast<const CallbackStructure<1> *>(id.getStructure());
			rawTypePtr = callbackSpec->argType;
			arity = callbackSpec->arity;

			while(arity--) {
				typeIdList.push_back(NBindID(*rawTypePtr));
				++rawTypePtr;
			}

			result = outTypeDetail.call<External>(
				static_cast<unsigned char>(placeholderFlag),
				NBindID(callbackSpec->returnType),
				typeIdList
			);

			break;

		default:
			paramSpec = static_cast<const ParamStructure *>(id.getStructure());

			result = outTypeDetail.call<External>(
				static_cast<unsigned char>(placeholderFlag),
				NBindID(paramSpec->target)
			);

			break;
	}

	return(result);
}
