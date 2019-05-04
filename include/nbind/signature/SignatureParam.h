// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

#if defined(BUILDING_NODE_EXTENSION)

struct SignatureParam {

	template <typename InfoType>
	static SignatureParam *get(const InfoType &args) {
		return(
			static_cast<SignatureParam *>(
				v8::Local<v8::External>::Cast(
					args.Data()
				)->Value()
			)
		);
	}

	union {
		// Distinguish methods with identical signatures.
		unsigned int methodNum = 0;
		unsigned int getterNum;
	};

	union {
		// Distinguish overloaded method signatures with matching names.
		unsigned int overloadNum = 0;
		unsigned int setterNum;
	};

};

#endif // BUILDING_NODE_EXTENSION

} // namespace
