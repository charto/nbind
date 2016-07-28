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
