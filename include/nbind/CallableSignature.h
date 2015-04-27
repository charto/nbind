// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Templated static class for each different function call signature exposed by the
// Node.js plugin. Used to pass arguments and return values between C++ and Node.js.
// Everything must be static because the V8 JavaScript engine wants a single
// function pointer to call.

template <typename FunctionType, typename SignatureData>
class CallableSignature {

public:

	struct FunctionInfo {

		FunctionInfo(const char *name, FunctionType func):name(name), func(func) {}

		const char *name;
		FunctionType func;

	};

	struct SignatureInfo {
		std::vector<struct FunctionInfo> funcVect;
		SignatureData data;
	};

	static FunctionType getFunction(unsigned int num) {
		return(signatureStore().funcVect[num].func);
	}

//	TODO: Maybe this function is useful for better error messages.
//	static const char *getMethodName(unsigned int num) {
//		return(signatureStore().funcVect[num].name);
//	}

	static unsigned int addFunction(const char *name, FunctionType func) {
		auto &funcVect = signatureStore().funcVect;

		funcVect.emplace_back(name, func);

		return(funcVect.size() - 1);
	}

protected:

	static SignatureInfo &signatureStore() {
		static SignatureInfo signatureInfo;
		return(signatureInfo);
	}

};

} // namespace
