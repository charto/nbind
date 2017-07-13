// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles type conversion of C++ standard library types
// to / from JavaScript.

import { setEvil, prepareNamespace } from 'emscripten-library-decorator';
import { _nbind as _globals } from './Globals';
import { _nbind as _type } from './BindingType';
import { _nbind as _resource } from './Resource';
import { TypeSpecWithParam, PolicyTbl } from '../Type';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {
	export const Pool = _globals.Pool;
	export const BindType = _type.BindType;
}

export namespace _nbind {

	export let resources: typeof _resource.resources;

	export function pushArray(arr: any[], type: ArrayType) {
		if(!arr) return(0);

		const length = arr.length;

		if((type.size || type.size === 0) && length < type.size) {
			throw(new Error('Type mismatch'));
		}

		const ptrSize = type.memberType.ptrSize;
		const result = Pool.lalloc(4 + length * ptrSize);

		HEAPU32[result / 4] = length;

		const heap = type.memberType.heap;
		let ptr = (result + 4) / ptrSize;

		const wireWrite = type.memberType.wireWrite;
		let num = 0;

		if(wireWrite) {
			while(num < length) {
				heap[ptr++] = wireWrite(arr[num++]);
			}
		} else {
			while(num < length) {
				heap[ptr++] = arr[num++];
			}
		}

		return(result);
	}

	export function popArray(ptr: number, type: ArrayType) {
		if(ptr === 0) return(null);

		const length = HEAPU32[ptr / 4];
		const arr = new Array(length);

		const heap = type.memberType.heap;
		ptr = (ptr + 4) / type.memberType.ptrSize;

		const wireRead = type.memberType.wireRead;
		let num = 0;

		if(wireRead) {
			while(num < length) {
				arr[num++] = wireRead(heap[ptr++]);
			}
		} else {
			while(num < length) {
				arr[num++] = heap[ptr++];
			}
		}

		return(arr);
	}

	export class ArrayType extends BindType {
		constructor(spec: TypeSpecWithParam) {
			super(spec);

			this.memberType = spec.paramList[0] as _type.BindType;
			if(spec.paramList[1]) this.size = spec.paramList[1] as number;
		}

		wireRead = (arg: number) => popArray(arg, this);
		wireWrite = (arg: any) => pushArray(arg, this);

		// Optional type conversion code
		/*
		makeWireRead = (expr: string, convertParamList: any[], num: number) => {
			convertParamList[num] = this;
			return('_nbind.popArray(' + expr + ',convertParamList[' + num + '])');
		};
		makeWireWrite = (expr: string, convertParamList: any[], num: number) => {
			convertParamList[num] = this;
			return('_nbind.pushArray(' + expr + ',convertParamList[' + num + '])');
		};
		*/

		readResources = [ resources.pool ];
		writeResources = [ resources.pool ];

		memberType: _type.BindType;
		size: number;
	}

	export function pushString(str: string, policyTbl?: PolicyTbl) {
		if(str === null || str === undefined) {
			if(policyTbl && policyTbl['Nullable']) {
				str = '';
			} else throw(new Error('Type mismatch'));
		}

		if(policyTbl && policyTbl['Strict']) {
			if(typeof(str) != 'string') throw(new Error('Type mismatch'));
		} else str = str.toString();

		const length = Module.lengthBytesUTF8(str);

		// 32-bit length, string and a zero terminator
		// (stringToUTF8Array insists on adding it)

		const result = Pool.lalloc(4 + length + 1);

		HEAPU32[result / 4] = length;
		Module.stringToUTF8Array(str, HEAPU8, result + 4, length + 1);

		return(result);
	}

	export function popString(ptr: number) {
		if(ptr === 0) return(null);

		const length = HEAPU32[ptr / 4];

		return(Module.Pointer_stringify(ptr + 4, length));
	}

	export class StringType extends BindType {
		makeWireWrite(expr: string, policyTbl: PolicyTbl) {
			return((arg: any) => pushString(arg, policyTbl));
		}

		wireRead = popString;
		wireWrite = pushString;

		readResources = [ resources.pool ];
		writeResources = [ resources.pool ];
	}

	@prepareNamespace('_nbind')
	export class _ {} // tslint:disable-line:class-name
}
