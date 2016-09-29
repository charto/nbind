// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import {Reflect, BindType, BindMethod, BindProperty} from './reflect';
import {TypeFlags} from './Type';

const nameTbl: { [key: string]: string } = {
	'void': 'void',
	'bool': 'boolean',
	'cbFunction &': '(...args: any[]) => any',
	'std::string': 'string',
	'External': 'any',
	'Buffer': 'number[] | ArrayBuffer | DataView | Uint8Array | Buffer',
	'Int64': 'number' // | Int64 (interface)?
};

type PolicyTbl = { [key: string]: boolean };

function formatType(bindType: BindType, policyTbl: PolicyTbl = {}): string {
	const flags = bindType.flags;
	const kind = flags & TypeFlags.kindMask;
	const refKind = flags & TypeFlags.refMask;

	function formatSubType() {
		return(formatType(bindType.spec.paramList![0] as BindType, policyTbl));
	}

	if(flags & TypeFlags.isConst) return(formatSubType());

	switch(kind) {
		case TypeFlags.isArithmetic:
			return('number');

		case TypeFlags.isClass:
			return(
				(
					refKind ?
					formatSubType() :
					bindType.name
				) + (
					// Objects passed by pointer (not value or reference)
					// may be nullable.
					policyTbl['Nullable'] && (
						refKind == TypeFlags.isPointer ||
						refKind == TypeFlags.isSharedPtr ||
						refKind == TypeFlags.isUniquePtr
					) ? ' | null' : ''
				)
			);

		case TypeFlags.isVector:
		case TypeFlags.isArray:
			return(formatSubType() + '[]');

		case TypeFlags.isCString:
			return('string' + (policyTbl['Nullable'] ? ' | null' : ''));

		case TypeFlags.isString:
			return('string');

		case TypeFlags.isOther:
			return(nameTbl[bindType.name] || 'any');

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

export function dump(reflect: Reflect) {
	const classCodeList = [
		'export class NBindBase { free?(): void }'
	];
	let indent: string;
	let staticPrefixCC: string;
	let staticPrefixJS: string;

	for(let bindClass of reflect.classList.concat([reflect.globalScope])) {
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

		if(indent) {
			classCode = (
				'export class ' + bindClass.name + ' extends NBindBase {' +
				(classCode ? '\n' + classCode + '\n' : '') +
				'}'
			);
		}

		classCodeList.push(classCode);
	}

	return(classCodeList.join('\n\n') + '\n');
}
