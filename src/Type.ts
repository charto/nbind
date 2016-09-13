// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

export var {
	Type, makeType, structureNameList
} = typeModule(typeModule);

export type PolicyTbl = { [name: string]: boolean };

export interface TypeSpec {
	id: number;
	name: string;
	flags: TypeFlags;

	ptrSize?: number;
	paramList?: any[];
}

export interface TypeClass extends TypeSpec {
	toString?(): string;

	makeWireRead?(expr: string, convertParamList?: any[], num?: number): string;
	makeWireWrite?(
		expr: string,
		policyTbl?: PolicyTbl,
		convertParamList?: any[],
		num?: number
	): string | ((arg: any) => number);

	wireRead?: (arg: number) => any;
	wireWrite?: (arg: any) => number;

	spec?: TypeSpec;
}

// These must match C++ enum StructureType in TypeID.h

export const enum StructureType {
	raw = 0,
	constant,
	pointer,
	vector,
	array
}

const enum Base {
	ref = 2,
	kind = 16,
	num = 128
}

export const enum TypeFlags {
	isConst = 1,

	refMask = Base.ref * 7,
	isPointer = Base.ref * 1,
	isReference = Base.ref * 2,
	isRvalueRef = Base.ref * 3,
	isSharedPtr = Base.ref * 4,
	isUniquePtr = Base.ref * 5,

	kindMask = Base.kind * 7,
	isPrimitive = Base.kind * 1,
	isClass = Base.kind * 2,
	isVector = Base.kind * 3,
	isArray = Base.kind * 4,
	isOther = Base.kind * 5,

	numMask = Base.num * 3,
	isUnsigned = Base.num * 1,
	isSignless = Base.num * 2,
	isFloat = Base.num * 3
}

/* tslint:disable:no-shadowed-variable */
export function typeModule(self: any) {

	// Printable name of each StructureType.

	const structureNameList = [
		'',
		'const X',
		'X *',
		'std::vector<X>',
		'std::array<X, Y>'
	];

	function makeType(spec: TypeSpec) {
		// const kind = spec.flags & TypeFlags.kindMask;
	}

	class Type implements TypeClass {
		constructor(spec: TypeSpec) {
			this.id = spec.id;
			this.name = spec.name;
			this.flags = spec.flags;
			this.spec = spec;
		}

		toString() {
			return(this.name);
		}

		id: number;
		name: string;
		flags: TypeFlags;
		spec: TypeSpec;
	}

	const output = {
		Type: Type as { new(spec: TypeSpec): TypeClass },
		makeType: makeType,
		structureNameList: structureNameList
	};

	self.output = output;

	return((self.output || output) as typeof output);
}
