// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

export var {
	Type, makeType, structureList
} = typeModule(typeModule);

export type PolicyTbl = { [name: string]: boolean };

export interface TypeSpec {
	[key: string]: any;

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
	isCString = Base.kind * 5,
	isString = Base.kind * 6,
	isOther = Base.kind * 7,

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

export type MakeTypeTbl = { [flags: number]: { new(spec: TypeSpec): TypeClass } };

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

	function reportProblem(
		problem: string,
		id: number,
		kind: string,
		structureType: StructureType,
		place: string
	) {
		throw(new Error(
			problem + ' type ' +
			kind.replace('X', id + '?') +
			(structureType ? ' with flag ' + structureType : '') +
			' in ' + place
		));
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

		tbl: MakeTypeTbl,

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

		if(problem) reportProblem(problem, id, kind, structureType, place);

		const subId = result.paramList[0];
		const subType = getType(subId) || getComplexType(
			subId,
			place,
			kind,
			structure,
			getType,
			queryType,

			tbl,

			depth + 1
		);

		let type: TypeClass;
		const name = applyStructure(
			structure[1], structure[0],
			subType.name, subType.flags
		);

		console.log(new Array(depth).join(' ') + name + ' contains ' + subType.name + ' kind ' + kind); // tslint:disable-line

		let addFlags: TypeFlags;
		let spec: TypeSpec = {
			flags: TypeFlags.isOther,
			id: id,
			name: name,
			paramList: [subType]
		};

		switch(result.placeholderFlag) {
			case StructureType.constant:
				spec = subType.spec;
				addFlags = TypeFlags.isConst;
				break;

			case StructureType.pointer:
				if((subType.flags & TypeFlags.kindMask) == TypeFlags.isPrimitive && subType.ptrSize == 1) {
					spec.flags = TypeFlags.isCString;
				} else {
					// reportProblem('Unsupported', id, kind, structureType, place);
				}
				break;

			case StructureType.reference:
				// reportProblem('Unsupported', id, kind, structureType, place);
				break;

			case StructureType.vector:
				spec.flags = TypeFlags.isVector;
				break;

			case StructureType.array:
				spec.flags = TypeFlags.isArray;
				spec.paramList.push(result.paramList[1]);
				break;

			default:
				break;
		}

		type = makeType(tbl, spec, addFlags);

		return(type);
	}

	function makeType(tbl: MakeTypeTbl, spec: TypeSpec, addFlags?: TypeFlags) {
		const flags = spec.flags;
		// const refKind = flags & TypeFlags.refMask;
		let kind = flags & TypeFlags.kindMask;

		if(addFlags) {
			const newSpec: TypeSpec = {} as TypeSpec;

			for(let key of Object.keys(spec)) newSpec[key] = spec[key];

			spec = newSpec;
			spec.flags |= addFlags;
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

		return(new tbl[kind](spec));
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
