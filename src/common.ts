// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// These must match C++ enum SignatureType in BaseSignature.h

export enum SignatureType {
	func = 0,
	method,
	getter,
	setter,
	construct
}

// These must match C++ enum StructureType in TypeID.h

export enum StructureType {
	raw = 0,
	vector,
	array
}

export function removeAccessorPrefix(name: string) {
	// The C++ side gives the same name to getters and setters.
	const prefixMatcher = /^[Gg]et_?([A-Z]?)/;

	return(name.replace(
		prefixMatcher,
		(match: string, initial: string) => initial.toLowerCase()
	));
}
