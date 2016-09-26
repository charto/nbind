// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import {Binding} from './nbind';
import {SignatureType, removeAccessorPrefix} from './common';
import {
	typeModule,
	TypeFlags,
	TypeSpec,
	TypeSpecWithName,
	TypeClass,
	MakeTypeTbl
} from './Type';

const { Type, makeType, getComplexType } = typeModule(typeModule);

export type TypeBase = TypeClass;
export const TypeBase = Type as { new(spec: TypeSpec): TypeClass };

class NBindID {
	constructor(id: number) {
		this.id = id;
	}

	fromJS(output: (id: number) => void) {
		output(this.id);
	}

	toString() {
		return('' + this.id);
	}

	id: number;
}

const typeIdTbl: { [id: number]: BindType } = {};
const typeNameTbl: { [name: string]: BindType } = {};

export class BindType extends TypeBase {
	constructor(spec: TypeSpecWithName) {
		super(spec);

		typeIdTbl[spec.id] = this;
		typeNameTbl[spec.name] = this;
	}
}

export class BindClass extends BindType {
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

	addProperty(name: string, kind: SignatureType, typeList: BindType[], policyList: string[]) {
		name = removeAccessorPrefix(name);

		let bindProperty = this.propertyTbl[name];

		if(!bindProperty) {
			bindProperty = new BindProperty(this, name);

			this.propertyTbl[name] = bindProperty;
			this.propertyList.push(bindProperty);
		}

		if(kind == SignatureType.getter) {
			// Property type is getter's return type.
			bindProperty.makeReadable(typeList[0]);
		} else {
			// Property type is type of setter's first argument.
			bindProperty.makeWritable(typeList[1]);
		}
	}

	name: string;

	methodTbl: { [name: string]: BindMethod } = {};
	methodList: BindMethod[] = [];

	propertyTbl: { [name: string]: BindProperty } = {};
	propertyList: BindProperty[] = [];
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

	toString() {
		return(
			(
				this.name ?
				this.returnType + ' ' + this.name :
				this.bindClass.name
			) +
			'(' + this.argTypeList.join(', ') + ')'
		);
	}

	bindClass: BindClass;
	name: string;

	returnType: BindType;
	argTypeList: BindType[];

	policyList: string[];

	isStatic: boolean;
}

export class BindProperty {
	constructor(
		bindClass: BindClass,
		name: string
	) {
		this.bindClass = bindClass;
		this.name = name;
	}

	makeReadable(bindType: BindType) {
		this.bindType = bindType;
		this.isReadable = true;
	}

	makeWritable(bindType: BindType) {
		this.bindType = bindType;
		this.isWritable = true;
	}

	toString() {
		return(this.bindType + ' ' + this.name);
	}

	bindClass: BindClass;
	name: string;
	bindType: BindType;

	isReadable = false;
	isWritable = false;
}

const makeTypeTbl: MakeTypeTbl = {
	[TypeFlags.isPrimitive]: BindType,
	[TypeFlags.isBig]: BindType,
	[TypeFlags.isClass]: BindClass,
	[TypeFlags.isClassPtr]: BindType,
	[TypeFlags.isSharedClassPtr]: BindType,
	[TypeFlags.isVector]: BindType,
	[TypeFlags.isArray]: BindType,
	[TypeFlags.isCString]: BindType,
	[TypeFlags.isOther]: BindType
};

function getType(id: number) {
	return(typeIdTbl[id]);
}

export class Reflect {
	constructor(binding: Binding<any>) {
		this.binding = binding;

		// Bind value type on Emscripten target
		// (equivalent Node.js type has no toJS method).
		binding.bind('NBindID', NBindID);

		binding.reflect(
			this.readPrimitive.bind(this),
			this.readType.bind(this),
			this.readClass.bind(this),
			this.readMethod.bind(this)
		);
	}

	private queryType(id: number) {
		let result: {placeholderFlag: number, paramList: number[]} | undefined;

		// TODO: just wrap the call in return() and remove the
		// result variable. Requires support for passing any
		// JavaScript type through C++.

		this.binding.queryType(id, (kind: number, target: number, param: number) => {
			result = {
				paramList: [target, param],
				placeholderFlag: kind
			};
		});

		return(result);
	}

	private readPrimitive(id: number, size: number, flags: number) {
		makeType(makeTypeTbl, {
			flags: TypeFlags.isPrimitive | flags,
			id: id,
			ptrSize: size
		});
	}

	private readType(id: number, name: string) {
		makeType(makeTypeTbl, {
			flags: TypeFlags.isOther,
			id: id,
			name: name
		});
	}

	private readClass(id: number, name: string) {
		this.classList.push(makeType(makeTypeTbl, {
			flags: TypeFlags.isClass,
			id: id,
			name: name
		}) as BindClass);
	}

	private readMethod(
		classId: number,
		name: string,
		kind: SignatureType,
		typeIdList: number[],
		policyList: string[]
	) {
		let bindClass = typeIdTbl[classId] as BindClass;

		if(!bindClass) {
			if(!this.globalScope) {
				bindClass = new BindClass({flags: TypeFlags.none, id: classId, name: 'global'});
				this.globalScope = bindClass;
			} else {
				throw(new Error('Unknown class ID ' + classId + ' for method ' + name));
			}
		}

		const typeList = typeIdList.map((id: number) => getComplexType(
			id,
			makeTypeTbl,
			getType,
			this.queryType.bind(this),
			'reflect ' + bindClass.name + '.' + name
		));

		switch(kind) {
			case SignatureType.construct:
			case SignatureType.func:
			case SignatureType.method:
				bindClass.addMethod(name, kind, typeList, policyList);
				break;

			case SignatureType.getter:
			case SignatureType.setter:
				bindClass.addProperty(name, kind, typeList, policyList);
				break;

			default:
				break;
		}
	}

	dumpPseudo() {
		const classCodeList: string[] = [];
		let indent: string;
		let staticPrefix: string;

		const skipNameTbl: { [key: string]: boolean } = {
			'Int64': true,
			'NBind': true,
			'NBindID': true
		};

		for(let bindClass of this.classList.reverse().concat([this.globalScope])) {
			if(skipNameTbl[bindClass.name]) continue;

			if((bindClass.flags & TypeFlags.kindMask) == TypeFlags.isClass) {
				indent = '\t';
				staticPrefix = 'static ';
			} else {
				indent = '';
				staticPrefix = '';
			}

			const methodCode = bindClass.methodList.reverse().map((method: BindMethod) => (
				indent +
				(method.name && method.isStatic ? staticPrefix : '') +
				method + ';' +
				(
					method.policyList.length ?
					' // ' + method.policyList.join(', ') :
					''
				)
			)).join('\n');

			const propertyCode = bindClass.propertyList.reverse().map((property: BindProperty) => (
				indent + property + ';' +
				(
					!(property.isReadable && property.isWritable) ?
					' // ' + (property.isReadable ? 'Read-only' : 'Write-only') :
					''
				)
			)).join('\n');

			let classCode = (
				methodCode +
				(methodCode && propertyCode ? '\n\n' : '') +
				propertyCode
			);

			if(indent) {
				classCode = (
					'class ' + bindClass.name + ' {' +
					(classCode ? '\n' + classCode + '\n' : '') +
					'};'
				);
			}

			classCodeList.push(classCode);
		}

		return(classCodeList.join('\n\n') + '\n');
	}

	binding: Binding<any>;

	globalScope: BindClass;
	typeList: BindType[] = [];
	classList: BindClass[] = [];
}
