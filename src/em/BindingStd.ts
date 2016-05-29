// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles type conversion of C++ standard library types
// to / from JavaScript.

import {setEvil, prepareNamespace} from 'emscripten-library-decorator';
import {_nbind as _globals} from './Globals';
import {_nbind as _type} from './BindingType';
import {_nbind as _resource} from './Resource';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {
	export var Pool = _globals.Pool;
	export var BindType = _type.BindType;
}

export namespace _nbind {

	export var resources: typeof _resource.resources;

	export function pushArray(arr: any[], type: ArrayType) {
		if(!arr) return(0);

		var length = arr.length;

		if((type.size || type.size === 0) && length < type.size) {
			throw(new Error('Type mismatch'));
		}

		var ptrSize = type.memberType.ptrSize;
		var result = Pool.lalloc(4 + length * ptrSize);

		HEAPU32[result / 4] = length;

		var heap = type.memberType.heap;
		var ptr = (result + 4) / ptrSize;

		var wireWrite = type.memberType.wireWrite;
		var num = 0;

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

		var length = HEAPU32[ptr / 4];
		var arr = new Array(length);

		var heap = type.memberType.heap;
		ptr = (ptr + 4) / type.memberType.ptrSize;

		var wireRead = type.memberType.wireRead;
		var num = 0;

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
		constructor(id: number, memberType: _type.BindType, size?: number) {
			super(
				id,
				((size || size === 0) ?
					'std::array<' + memberType.name + ', ' + size + '>' :
					'std::vector<' + memberType.name + '>'
				)
			);

			this.memberType = memberType;
			if(size) this.size = size;
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

	export function pushString(str: string) {
		if(str === null || str === undefined) return(0);
		str = str.toString();

		var length = Module.lengthBytesUTF8(str);

		// 32-bit length, string and a zero terminator
		// (stringToUTF8Array insists on adding it)

		var result = Pool.lalloc(4 + length + 1);

		HEAPU32[result / 4] = length;
		Module.stringToUTF8Array(str, HEAPU8, result + 4, length + 1);

		return(result);
	}

	export function popString(ptr: number) {
		if(ptr === 0) return(null);

		var length = HEAPU32[ptr / 4];

		return(Module.Pointer_stringify(ptr + 4, length));
	}

	export class StringType extends BindType {
		constructor(id: number, name: string) {
			super(id, name);
		}

		wireRead = popString;
		wireWrite = pushString;

		// Optional type conversion code
		// makeWireRead = (expr: string) => '_nbind.popString(' + expr + ')';
		// makeWireWrite = (expr: string) => '_nbind.pushString(' + expr + ')';

		readResources = [ resources.pool ];
		writeResources = [ resources.pool ];
	}

	@prepareNamespace('_nbind')
	export class _ {}
}
