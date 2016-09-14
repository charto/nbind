// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

export var {
	Type, makeType, structureList
} = typeModule(typeModule);

export type PolicyTbl = { [name: string]: boolean };

export interface TypeSpec {
	id: number;
	name?: string;
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

const enum Base {
	ref = 2,
	kind = 16,
	num = 128
}

export {Base as TypeFlagBase};

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

	numMask = Base.num * 15,
	isUnsigned = Base.num * 1,
	isSignless = Base.num * 2,
	isFloat = Base.num * 4,
	isBig = Base.num * 8
}

// These must match C++ enum StructureType in TypeID.h

export const enum StructureType {
	raw = 0,
	constant,
	pointer,
	reference,
	rvalue,
	vector,
	array,
	max
}

/* tslint:disable:no-shadowed-variable */
export function typeModule(self: any) {

	// Printable name of each StructureType.

	type Structure = [TypeFlags, string];
	const structureList: Structure[] = [
		[0, 'X'],
		[TypeFlags.isConst, 'const X'],
		[TypeFlags.isPointer, 'X *'],
		[TypeFlags.isReference, 'X &'],
		[TypeFlags.isRvalueRef, 'X &&'],
		[TypeFlags.isVector, 'std::vector<X>'],
		[TypeFlags.isArray, 'std::array<X, Y>']
	];

	function applyStructure(
		outerName: string,
		outerFlags: TypeFlags,
		innerName: string,
		innerFlags: TypeFlags,
		flip?: boolean
	) {
		if(outerFlags == TypeFlags.isConst) {
			const ref = innerFlags & TypeFlags.refMask;
			if(
				ref == TypeFlags.isPointer ||
				ref == TypeFlags.isReference ||
				ref == TypeFlags.isRvalueRef
			) outerName = 'X const';
		}

		let name: string;

		if(flip) {
			name = innerName.replace('X', outerName);
		} else {
			name = outerName.replace('X', innerName);
		}

		// Remove spaces between consecutive * and & characters.
		return(name.replace(/([*&]) (?=[*&])/g, '$1'));
	}

	function getComplexType(
		id: number,
		place: string,
		kind: string,
		prevStructure: Structure,
		getType: (id: number) => TypeClass,
		queryType: (id: number) => {
			placeholderFlag: number,
			paramList: number[]
		},

		BindType: any,
		PrimitiveType: any,
		CStringType: any,
		ArrayType: any,

		depth: number = 1 // tslint:disable-line
	) {
		const result = queryType(id);
		const structureType: StructureType = result.placeholderFlag;

		let structure = structureList[structureType];

		if(prevStructure && structure) {
			kind = applyStructure(
				prevStructure[1], prevStructure[0],
				kind, structure[0],
				true
			);
		}

		let problem: string;

		if(structureType == 0) problem = 'Unbound';
		if(structureType >= StructureType.max) problem = 'Corrupt';
		if(depth > 20) problem = 'Deeply nested';

		if(problem) {
			throw(new Error(
				problem + ' type ' +
				kind.replace('X', id + '?') +
				(structureType ? ' with flag ' + structureType : '') +
				' in ' + place
			));
		}

		const subId = result.paramList[0];
		const subType = getType(subId) || getComplexType(
			subId,
			place,
			kind,
			structure,
			getType,
			queryType,

			BindType,
			PrimitiveType,
			CStringType,
			ArrayType,

			depth + 1
		);

		let type: TypeClass;
		const name = applyStructure(
			structure[1], structure[0],
			subType.name, subType.flags
		);

		console.log(new Array(depth).join(' ') + name + ' contains ' + subType.name + ' kind ' + kind); // tslint:disable-line

		switch(result.placeholderFlag) {
			case StructureType.constant:
console.log('CONSTANT ' + subType + ' ' + id + ' ' + subId); // tslint:disable-line
// TODO: call makeType instead!
// type = new BindType({flags: TypeFlags.isConst, id: id, name: name});
type = subType;
				break;
			case StructureType.pointer:
console.log('POINTER ' + subType + ' ' + id + ' ' + subId); // tslint:disable-line
				if(subType instanceof PrimitiveType && subType.ptrSize == 1) {
					// TODO: call makeType instead!
					type = new CStringType(id, name);
				} else {
					type = new BindType({flags: TypeFlags.isPointer, id: id, name: name});
					// throw(new Error('Unsupported type ' + name + ' in ' + place));
				}
				break;
			case StructureType.reference:
console.log('REFERENCE ' + subType + ' ' + id + ' ' + subId); // tslint:disable-line
type = new BindType({flags: TypeFlags.isReference, id: id, name: name});
				// type = subType;
				break;
			case StructureType.vector:
			// TODO: call makeType instead!
				type = new ArrayType(id, name, subType);
				break;
			case StructureType.array:
				const size = result.paramList[1];
				// TODO: call makeType instead!
				type = new ArrayType(
					id,
					name.replace('Y', '' + size),
					subType,
					size
				);
				break;
			default:
				break;
		}

		return(type);
	}

	function makeType(tbl: any, spec: TypeSpec, addFlags?: TypeFlags) {
		const flags = spec.flags | addFlags;
		const refKind = flags & TypeFlags.refMask;
		let kind = flags & TypeFlags.kindMask;

		if(refKind) {
			// ...
		}

		if(kind == TypeFlags.isPrimitive) {
			if(flags & TypeFlags.isSignless) {
				spec.name = 'char';
			} else {
				spec.name = (
					(flags & TypeFlags.isUnsigned ? 'u' : '') +
					(flags & TypeFlags.isFloat ? 'float' : 'int') +
					(spec.ptrSize * 8 + '_t')
				);
			}

			if(spec.ptrSize == 8 && !(flags & TypeFlags.isFloat)) kind = TypeFlags.isBig;
		}

		// tslint:disable-next-line:unused-expression
		new tbl[kind](spec);
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
		getComplexType: getComplexType,
		makeType: makeType,
		structureList: structureList
	};

	self.output = output;

	return((self.output || output) as typeof output);
}
