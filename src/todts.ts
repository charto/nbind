// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import { Reflect, BindType, BindClass, BindMethod, BindProperty } from './reflect';
import { TypeFlags } from './Type';

// TypeScript alternatives to named C++ types,
// and a flag whether they need surrounding parentheses when nested.

const nameTbl: { [key: string]: [string, boolean] } = {
	'Buffer': ['number[] | ArrayBuffer | DataView | Uint8Array | Buffer', true],
	'External': ['any', false],
	'Int64': ['number', false], // | Int64 (interface)?
	'bool': ['boolean', false],
	'cbFunction &': ['(...args: any[]) => any', true],
	'std::string': ['string', false],
	'void': ['void', false]
};

type PolicyTbl = { [key: string]: boolean };

// tslint:disable-next-line:typedef
function formatType(bindType: BindType, policyTbl: PolicyTbl = {}, needParens = false): string {
	const flags = bindType.flags;
	const kind = flags & TypeFlags.kindMask;
	const refKind = flags & TypeFlags.refMask;

	// tslint:disable-next-line:typedef
	function formatSubType(needsParens: boolean, num = 0) {
		return(formatType(bindType.spec.paramList![num] as BindType, policyTbl, needsParens));
	}

	function addParens(name: string) {
		return(needParens ? '(' + name + ')' : name);
	}

	if(flags & TypeFlags.isConst) return(formatSubType(needParens));

	let isNullable = policyTbl['Nullable'];
	const argList: string[] = [];

	switch(kind) {
		case TypeFlags.isArithmetic:
			return('number');

		case TypeFlags.isClass:
			// Objects passed by pointer (not value or reference)
			// may be nullable.

			isNullable = isNullable && (
				refKind == TypeFlags.isPointer ||
				refKind == TypeFlags.isSharedPtr ||
				refKind == TypeFlags.isUniquePtr
			);

			needParens = needParens && isNullable;

			return(addParens(
				(refKind ? formatSubType(isNullable) : bindType.name) +
				(isNullable ? ' | null' : '')
			));

		case TypeFlags.isVector:
		case TypeFlags.isArray:
			return(addParens(formatSubType(true) + '[]'));

		case TypeFlags.isCString:
			return(isNullable ? addParens('string | null') : 'string');

		case TypeFlags.isString:
			return('string');

		case TypeFlags.isCallback:
			for(let num = 1; num < bindType.spec.paramList!.length; ++num) {
				argList.push('p' + (num - 1) + ': ' + formatSubType(false, num));
			}

			return(addParens(
				'(' + argList.join(', ') + ') => ' + formatSubType(true)
			));

		case TypeFlags.isOther:
			const spec = nameTbl[bindType.name];
			return(spec ? (spec[1] ? addParens(spec[0]) : spec[0]) : 'any');

		default:
			return('any');
	}
}

function formatMethod(method: BindMethod) {
	const policyTbl: PolicyTbl = {};

	for(let key of method.policyList) policyTbl[key] = true;

	const args = (
		'(' + method.argTypeList.map(
			(bindType: BindType, num: number) => 'p' + num + ': ' + formatType(bindType, policyTbl)
		).join(', ') + ')'
	);

	if(method.name) {
		// Most return types may be null.
		return(method.name + args + ': ' + formatType(method.returnType, { 'Nullable': true }) + ';');
	} else {
		return('constructor' + args + ';');
	}
}

function formatProperty(prop: BindProperty) {
	return(prop.name + ': ' + formatType(prop.bindType) + ';');
}

export function dump(options: { reflect: Reflect, shim?: boolean } ) {
	const classCodeList = [];
	let indent: string;
	let staticPrefixCC: string;
	let staticPrefixJS: string;
	let classList = options.reflect.classList;

	if(options.shim) {
		classCodeList.push('import { Buffer } from "nbind/dist/shim";');
	}

	classCodeList.push('export class NBindBase { free?(): void }');

	if(options.reflect.globalScope) {
		classList = classList.concat([options.reflect.globalScope]);
	}

	for(let bindClass of classList) {
		if(bindClass.isClass) {
			indent = '\t';
			staticPrefixCC = 'static ';
			staticPrefixJS = 'static ';
		} else {
			indent = '';
			staticPrefixCC = '';
			staticPrefixJS = 'export function ';
		}

		const methodBlock = bindClass.methodList.map((method: BindMethod) => (
			indent +
			'/** ' + (method.name && method.isStatic ? staticPrefixCC : '') +
			method + ';' + (
				method.policyList.length ?
				' -- ' + method.policyList.join(', ') :
				''
			) +
			' */\n' +
			indent + (method.name && method.isStatic ? staticPrefixJS : '') +
			formatMethod(method)
		)).join('\n\n');

		const propertyBlock = bindClass.propertyList.map((property: BindProperty) => (
			indent +
			'/** ' + property + ';' +
			(
				!(property.isReadable && property.isWritable) ?
				' -- ' + (property.isReadable ? 'Read-only' : 'Write-only') :
				''
			) +
			' */\n' +
			indent +
			formatProperty(property)
		)).join('\n\n');

		let classCode = (
			methodBlock +
			(methodBlock && propertyBlock ? '\n\n' : '') +
			propertyBlock
		);

		let superClass = 'NBindBase';

		if(bindClass.superList.length == 1) {
			superClass = bindClass.superList[0].name;
		} else if(bindClass.superList.length > 1) {
			superClass = '_' + bindClass.name;

			classCodeList.push(
				'export interface ' + superClass + ' extends ' + bindClass.superList.map(
					(superClassEntry: BindClass) => superClassEntry.name
				).join(', ') + ' {}\n' +
				'export var ' + superClass + ': { new(): ' + superClass + ' };'
			);
		}

		if(indent) {
			classCode = (
				'export class ' + bindClass.name + ' extends ' + superClass + ' {' +
				(classCode ? '\n' + classCode + '\n' : '') +
				'}'
			);
		}

		classCodeList.push(classCode);
	}

	return(classCodeList.join('\n\n') + '\n');
}
