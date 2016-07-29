// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import {Binding} from './nbind';
import {SignatureType, StructureType, removeAccessorPrefix} from './common';

export class BindType {
	constructor(id: number, name: string) {
		this.id = id;
		this.name = name;
	}

	toString() {
		return(this.name);
	}

	id: number;
	name: string;

	isPointer = false;
	isClass = false;

	target: BindType;
}

export class BindPrimitive extends BindType {

	/** Partially duplicates _nbind_register_primitive. */

	constructor(id: number, size: number, flag: number) {
		const isSignless = flag & 16;
		const isFloat    = flag & 2;
		const isUnsigned = flag & 1;

		let name = '';

		if(isSignless) {
			name += 'char';
		} else {
			name += (
				(isUnsigned ? 'u' : '') +
				(isFloat ? 'float' : 'int') +
				(size * 8 + '_t')
			);
		}

		super(id, name);

		this.isSignless = !!isSignless;
		this.isFloat = !!isFloat;
		this.isUnsigned = !!isUnsigned;
	}

	isSignless: boolean;
	isFloat: boolean;
	isUnsigned: boolean;
}

export class BindVector extends BindType {
	constructor(id: number, target: BindType) {
		super(id, 'std::vector<' + target.name + '>');

		this.target = target;
	}

	isVector = true;

	target: BindType;
}

export class BindArray extends BindType {
	constructor(id: number, target: BindType, length: number) {
		super(id, 'std::array<' + target.name + ', ' + length + '>');

		this.target = target;
		this.length = length;
	}

	isVector = true;

	target: BindType;
	length: number;
}

export class BindPointer extends BindType {
	constructor(id: number, target: BindType, isConst?: boolean) {
		super(id, (isConst ? 'const ' : '') + target.name.replace(/([^*])$/, '$1 ') + '*');

		this.isConst = !!isConst;
		this.target = target;
	}

	isPointer = true;
	isConst: boolean;

	target: BindType;
}

export class BindClass extends BindType {
	constructor(id: number, name: string) {
		super(id, name);
	}

	addMethod(name: string, kind: SignatureType, typeList: BindType[], policyList: string[]) {
		const bindMethod = new BindMethod(
			this,
			name,
			typeList[0],
			typeList.slice(1),
			policyList,
			kind == SignatureType.func
		);

		this.methodTbl[name] = bindMethod;
		this.methodList.push(bindMethod);
	}

	isClass = true;

	methodTbl: { [name: string]: BindMethod } = {};
	methodList: BindMethod[] = [];

	// propertyTbl: { [name: string]: BindProperty } = {};
	// propertyList: BindProperty = [];
}

export class BindMethod {
	constructor(
		bindClass: BindClass,
		name: string,
		returnType: BindType,
		argTypeList: BindType[],
		policyList: string[],
		isStatic: boolean
	) {
		this.bindClass = bindClass;
		this.name = name;
		this.returnType = returnType;
		this.argTypeList = argTypeList;
		this.policyList = policyList;
		this.isStatic = isStatic;
	}

	bindClass: BindClass;
	name: string;

	returnType: BindType;
	argTypeList: BindType[];

	policyList: string[];

	isStatic: boolean;
}

export class Reflect {
	constructor(binding: Binding<any>) {
		const globalScope = new BindClass(0, 'global');

		this.registerType(globalScope);
		this.classList.push(globalScope);

		this.binding = binding;

		binding.reflect(
			this.readPrimitive.bind(this),
			this.readType.bind(this),
			this.readClass.bind(this),
			this.readMethod.bind(this)
		);
	}

	private registerType(bindType: BindType) {
		this.typeIdTbl[bindType.id] = bindType;
		this.typeNameTbl[bindType.name] = bindType;

		this.typeList.push(bindType);
	}

	private getType(id: number) {
		let bindType = this.typeIdTbl[id];

		if(bindType) return(bindType);

		this.binding.queryType(id, (kind: number, ...args: any[]) => {
			switch(kind) {
				case StructureType.vector:
					bindType = new BindVector(id, this.getType(args[0]));
					break;

				case StructureType.array:
					bindType = new BindArray(id, this.getType(args[0]), args[1]);
					break;

				default:
					// throw(new Error('Undefined type ' + id));
					console.error('Undefined type ' + id); // tslint:disable-line
			}
		});

		return(bindType);
	}

	private readPrimitive(id: number, size: number, flag: number) {
		const isConst = flag & 8;
		const isPointer = flag & 4;
		let bindType: BindPrimitive;

		if(isPointer) {
			bindType = this.primitiveTbl[size + '/' + (flag & ~12)];

			this.registerType(new BindPointer(id, bindType, !!isConst));
		} else {
			bindType = new BindPrimitive(id, size, flag);

			this.primitiveTbl[size + '/' + flag] = bindType;
			this.registerType(bindType);
		}
	}

	private readType(id: number, name: string) {
		this.registerType(new BindType(id, name));
	}

	private readClass(
		id: number,
		ptrId: number,
		constPtrId: number,
		name: string
	) {
		const bindClass = new BindClass(id, name);

		this.registerType(bindClass);
		this.classList.push(bindClass);

		this.registerType(new BindPointer(ptrId, bindClass));
		this.registerType(new BindPointer(constPtrId, bindClass, true));
	}

	private readMethod(
		classId: number,
		name: string,
		kind: SignatureType,
		typeIdList: number[],
		policyList: string[]
	) {
		const bindClass = this.typeIdTbl[classId] as BindClass;

		if(!bindClass) {
			throw(new Error('Unknown class ID ' + classId + ' for method ' + name));
		} else {
			const typeList = typeIdList.map((id: number) => this.getType(id));

			switch(kind) {
				case SignatureType.func:
				case SignatureType.method:
					bindClass.addMethod(name, kind, typeList, policyList);
					break;

				case SignatureType.getter:
				case SignatureType.setter:
					console.error(removeAccessorPrefix(name)); // tslint:disable-line
					break;

				default:
					break;
			}
		}
	}

	/* tslint:disable */

	dumpPseudo() {
		const lineList: string[] = [];
		let indent: string;
		let staticPrefix: string;

		for(let bindClass of this.classList.reverse()) {
			if(bindClass.id) {
				console.log('class ' + bindClass.name + ' {');
				indent = '\t';
				staticPrefix = 'static ';
			} else {
				indent = '';
				staticPrefix = '';
			}

			for(let method of bindClass.methodList.reverse()) {
				console.log(
					indent +
					(method.isStatic ? staticPrefix : '') +
					method.returnType + ' ' +
					method.name + '(' + method.argTypeList.join(', ') + ')' +
					';' +
					(
						method.policyList.length ?
						' // ' + method.policyList.join(', ') :
						''
					)
				);
			}

			if(indent) console.log('};');
			console.log('');
		}

		return(lineList.join('\n'));
	}

	/* tslint:enable */

	binding: Binding<any>;

	typeIdTbl: { [id: number]: BindType } = {};
	typeNameTbl: { [name: string]: BindType } = {};

	primitiveTbl: { [key: string]: BindPrimitive } = {};

	typeList: BindType[] = [];
	classList: BindClass[] = [];
}
