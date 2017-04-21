// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

export const {
	Type, makeType, structureList
} = typeModule(typeModule);

export type PolicyTbl = { [name: string]: boolean };

export interface TypeSpec {
	[key: string]: any;

	id: number;
	name?: string;
	flags: TypeFlags;

	ptrSize?: number;
	paramList?: (TypeClass | number)[];
}

export interface TypeSpecWithName extends TypeSpec {
	name: string;
}

export interface TypeSpecWithParam extends TypeSpecWithName {
	paramList: (TypeClass | number)[];
}

export interface TypeSpecWithSize extends TypeSpecWithName {
	ptrSize: number;
}

export interface TypeClass extends TypeSpec {
	toString?(): string;

	makeWireRead?(expr: string, convertParamList?: any[], num?: number): string;
	makeWireWrite?(
		expr: string,
		policyTbl: PolicyTbl | null,
		convertParamList?: any[],
		num?: number
	): boolean | string | ((arg: any) => number | boolean);

	wireRead?: (arg: number) => any;
	wireWrite?: (arg: any) => number;

	spec: TypeSpec;
	name: string;
}

export const enum TypeFlagBase {
	flag = 1,
	num = flag * 8,
	ref = num * 16,
	kind = ref * 8
}

// These must match Policy.h.

export const enum TypeFlags {
	none = 0,

	flagMask = TypeFlagBase.flag * 3,
	isConst = TypeFlagBase.flag * 1,
	isValueObject = TypeFlagBase.flag * 2,
	isMethod = TypeFlagBase.flag * 4,

	numMask = TypeFlagBase.num * 15,
	isUnsigned = TypeFlagBase.num * 1,
	isSignless = TypeFlagBase.num * 2,
	isFloat = TypeFlagBase.num * 4,
	isBig = TypeFlagBase.num * 8,

	refMask = TypeFlagBase.ref * 7,
	isPointer = TypeFlagBase.ref * 1,
	isReference = TypeFlagBase.ref * 2,
	isRvalueRef = TypeFlagBase.ref * 3,
	isSharedPtr = TypeFlagBase.ref * 4,
	isUniquePtr = TypeFlagBase.ref * 5,

	kindMask = TypeFlagBase.kind * 15,
	isArithmetic = TypeFlagBase.kind * 1,
	isClass = TypeFlagBase.kind * 2,
	isClassPtr = TypeFlagBase.kind * 3,
	isSharedClassPtr = TypeFlagBase.kind * 4,
	isVector = TypeFlagBase.kind * 5,
	isArray = TypeFlagBase.kind * 6,
	isCString = TypeFlagBase.kind * 7,
	isString = TypeFlagBase.kind * 8,
	isCallback = TypeFlagBase.kind * 9,
	isOther = TypeFlagBase.kind * 10
}

export const enum StateFlags {
	none = 0,

	isPersistent = 1,
	isDeleted = 2
}

// These must match C++ enum StructureType in TypeID.h

export const enum StructureType {
	none = 0,
	constant,
	pointer,
	reference,
	rvalue,
	shared,
	unique,
	vector,
	array,
	callback,
	max
}

/* tslint:disable:no-shadowed-variable */
export function typeModule(self: any) {

	// Parameter count and printable name of each StructureType.

	type Structure = [TypeFlags, number, string];
	const structureList: Structure[] = [
		[0, 1, 'X'],
		[TypeFlags.isConst, 1, 'const X'],
		[TypeFlags.isPointer, 1, 'X *'],
		[TypeFlags.isReference, 1, 'X &'],
		[TypeFlags.isRvalueRef, 1, 'X &&'],
		[TypeFlags.isSharedPtr, 1, 'std::shared_ptr<X>'],
		[TypeFlags.isUniquePtr, 1, 'std::unique_ptr<X>'],
		[TypeFlags.isVector, 1, 'std::vector<X>'],
		[TypeFlags.isArray, 2, 'std::array<X, Y>'],
		[TypeFlags.isCallback, -1, 'std::function<X (Y)>']
	];

	function applyStructure(
		outerName: string,
		outerFlags: TypeFlags,
		innerName: string,
		innerFlags: TypeFlags,
		param: string,
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
			name = innerName.replace('X', outerName).replace('Y', param);
		} else {
			name = outerName.replace('X', innerName).replace('Y', param);
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

	function getComplexType<BindType extends TypeClass>(
		id: number,
		constructType: (kind: TypeFlags, spec: TypeSpecWithName) => BindType,
		getType: (id: number) => BindType,
		queryType: (id: number) => {
			placeholderFlag: number,
			paramList: (number | number[])[]
		},
		place?: string,
		// C++ type name string built top-down, for printing helpful errors.
		kind = 'X', // tslint:disable-line
		// Outer type, used only for updating kind.
		prevStructure?: Structure | undefined, // tslint:disable-line
		depth: number = 1 // tslint:disable-line
	) {
		const result = getType(id);
		if(result) return(result);

		const query = queryType(id);
		const structureType: StructureType = query.placeholderFlag;

		let structure = structureList[structureType];

		if(prevStructure && structure) {
			kind = applyStructure(
				prevStructure[2], prevStructure[0],
				kind, structure[0],
				'?',
				true
			);
		}

		let problem: string | undefined;

		if(structureType == 0) problem = 'Unbound';
		if(structureType >= StructureType.max) problem = 'Corrupt';
		if(depth > 20) problem = 'Deeply nested';

		if(problem) reportProblem(problem, id, kind, structureType, place || '?');

		const subId = query.paramList[0] as number;
		const subType = getComplexType(
			subId,
			constructType,
			getType,
			queryType,
			place,
			kind,
			structure,
			depth + 1
		);

		let srcSpec: TypeSpec | undefined;
		let spec: TypeSpecWithParam = {
			flags: structure[0],
			id: id,
			name: '',
			paramList: [subType]
		};

		const argList: string[] = [];
		let structureParam = '?';

		switch(query.placeholderFlag) {
			case StructureType.constant:
				srcSpec = subType.spec;
				break;

			case StructureType.pointer:
				if(
					(subType.flags & TypeFlags.kindMask) == TypeFlags.isArithmetic &&
					subType.spec.ptrSize == 1
				) {
					spec.flags = TypeFlags.isCString;
					break;
				}

			// tslint:disable-next-line:no-switch-case-fall-through
			case StructureType.reference:
			// tslint:disable-next-line:no-switch-case-fall-through
			case StructureType.unique:
			// tslint:disable-next-line:no-switch-case-fall-through
			case StructureType.shared:
				srcSpec = subType.spec;

				if((subType.flags & TypeFlags.kindMask) != TypeFlags.isClass) {
					// reportProblem('Unsupported', id, kind, structureType, place);
				}
				break;

			case StructureType.array:
				structureParam = '' + (query.paramList[1] as number);
				spec.paramList.push(query.paramList[1] as number);
				break;

			case StructureType.callback:
				for(let paramId of (query.paramList[1] as number[])) {
					const paramType = getComplexType(
						paramId,
						constructType,
						getType,
						queryType,
						place,
						kind,
						structure,
						depth + 1
					);

					argList.push(paramType.name);
					spec.paramList.push(paramType);
				}

				structureParam = argList.join(', ');
				break;

			default:
				break;
		}

		spec.name = applyStructure(
			structure[2], structure[0],
			subType.name, subType.flags,
			structureParam
		);

		if(srcSpec) {
			for(let key of Object.keys(srcSpec)) {
				spec[key] = spec[key] || srcSpec[key];
			}

			spec.flags |= srcSpec.flags;
		}

		return(makeType(constructType, spec));
	}

	function makeType<BindType extends TypeClass>(
		constructType: (kind: TypeFlags, spec: TypeSpecWithName) => BindType,
		spec: TypeSpec
	) {
		const flags = spec.flags;
		const refKind = flags & TypeFlags.refMask;
		let kind = flags & TypeFlags.kindMask;

		if(!spec.name && kind == TypeFlags.isArithmetic) {
			if(spec.ptrSize == 1) {
				spec.name = (
					flags & TypeFlags.isSignless ?
					'' :
					(flags & TypeFlags.isUnsigned ? 'un' : '') + 'signed '
				) + 'char';
			} else {
				spec.name = (
					(flags & TypeFlags.isUnsigned ? 'u' : '') +
					(flags & TypeFlags.isFloat ? 'float' : 'int') +
					(spec.ptrSize! * 8 + '_t')
				);
			}
		}

		if(spec.ptrSize == 8 && !(flags & TypeFlags.isFloat)) kind = TypeFlags.isBig;
		if(kind == TypeFlags.isClass) {
			if(refKind == TypeFlags.isSharedPtr || refKind == TypeFlags.isUniquePtr) {
				kind = TypeFlags.isSharedClassPtr;
			} else if(refKind) kind = TypeFlags.isClassPtr;
		}

		return(constructType(kind, spec as TypeSpecWithName));
	}

	class Type implements TypeClass {
		constructor(spec: TypeSpecWithName) {
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
