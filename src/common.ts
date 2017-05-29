// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// These must match C++ enum SignatureType in BaseSignature.h

export const enum SignatureType {
	none = 0,
	func,
	method,
	getter,
	setter,
	construct
}

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
