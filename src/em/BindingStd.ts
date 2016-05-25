// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import {setEvil, prepareNamespace} from 'emscripten-library-decorator';
import {_nbind as _globals} from './Globals';
import {_nbind as _type} from './BindingType';
import {_nbind as _class} from './BindClass';
import {_nbind as _resource} from './Resource';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {
	export var BindType = _type.BindType;
}

export namespace _nbind {

	export var resources: typeof _resource.resources;

	export function pushArray(arr: any[], type: ArrayType) {
		if((type.size || type.size === 0) && arr.length < type.size) {
			throw(new Error('Type mismatch'));
		}

		return(0);
	}

	export function popArray(ptr: number, type: ArrayType) {
		if(ptr === 0) return(null);

		var length = HEAPU32[ptr / 4];
		var arr = new Array(length);

		ptr += 4;
		ptr /= type.memberType.ptrSize;
		var heap = type.memberType.heap;

		for(var num = 0; num < length; ++num) {
			arr[num] = heap[ptr++];
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

		makeWireRead = (expr: string, convertParamList: any[], num: number) => {
			convertParamList[num] = this;
			return('_nbind.popArray(' + expr + ',convertParamList[' + num + '])');
		};
		makeWireWrite = (expr: string, convertParamList: any[], num: number) => {
			convertParamList[num] = this;
			return('_nbind.pushArray(' + expr + ',convertParamList[' + num + '])');
		};

		readResources = [ resources.pool ];
		writeResources = [ resources.stack ];

		memberType: _type.BindType;
		size: number;
	}

	export function pushString(str: string) {
		if(str === null || str === undefined) return(0);
		str = str.toString();

		var length = Module.lengthBytesUTF8(str);

		// 32-bit length, string and a zero terminator
		// (stringToUTF8Array insists on adding it)

		var result = Runtime.stackAlloc(4 + length + 1);

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

		makeWireRead = (expr: string) => '_nbind.popString(' + expr + ')';
		makeWireWrite = (expr: string) => '_nbind.pushString(' + expr + ')';

		readResources = [ resources.pool ];
		writeResources = [ resources.stack ];
	}

	@prepareNamespace('_nbind')
	export class _ {}
}
