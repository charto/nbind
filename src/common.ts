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

export var structureNameList = [
	'const X',
	'X *',
	'std::vector<X>',
	'std::array<X, Y>'
];

export function removeAccessorPrefix(name: string) {
	// The C++ side gives the same name to getters and setters.
	const prefixMatcher = /^[Gg]et_?([A-Z]?([A-Z]?))/;

	return(name.replace(
		prefixMatcher,
		(match: string, initial: string, second: string) => (
			second ? initial : initial.toLowerCase()
		)
	));
}
