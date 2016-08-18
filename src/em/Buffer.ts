// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import {setEvil, prepareNamespace} from 'emscripten-library-decorator';
import {_nbind as _globals} from './Globals';
import {_nbind as _type} from './BindingType';
import {_nbind as _external} from './External';
import {_nbind as _resource} from './Resource';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {
	export var Pool = _globals.Pool;
	export var BindType = _type.BindType;
}

export namespace _nbind {

	export type PolicyTbl = _globals.PolicyTbl;

	export var registerExternal: typeof _external.registerExternal;
	export var externalList: typeof _external.externalList;

	export var resources: typeof _resource.resources;

	function getBuffer(
		buf: number[] | ArrayBuffer | DataView | Uint8Array
	): number[] | Uint8Array {
		if(buf instanceof ArrayBuffer) {
			return(new Uint8Array(buf));
		} else if(buf instanceof DataView) {
			return(new Uint8Array(buf.buffer, buf.byteOffset, buf.byteLength));
		} else return(buf);
	}

	export function pushBuffer(
		buf: number[] | ArrayBuffer | DataView | Uint8Array,
		policyTbl?: PolicyTbl
	) {
		if(buf === null || buf === undefined) {
			if(policyTbl && policyTbl['Nullable']) buf = [];
		}

		if(typeof(buf) != 'object') throw(new Error('Type mismatch'));

		const b = buf as any;
		const length = b.byteLength || b.length;
		if(!length && length !== 0 && b.byteLength !== 0) throw(new Error('Type mismatch'));

		const result = Pool.lalloc(8);
		const data = _malloc(length);
		let ptr = result / 4;

		HEAPU32[ptr++] = length;
		HEAPU32[ptr++] = data;
		HEAPU32[ptr++] = registerExternal(buf);

		HEAPU8.set(getBuffer(buf), data);

		return(result);
	}

	export class BufferType extends BindType {
		constructor(id: number, name: string) {
			super(id, name);
		}

		makeWireWrite = (expr: string, policyTbl: PolicyTbl) => (
			(arg: any) => pushBuffer(arg, policyTbl)
		);
		wireWrite = pushBuffer;

		readResources = [ resources.pool ];
		writeResources = [ resources.pool ];
	}

	/* tslint:disable */
	export function commitBuffer(num: number, data: number, length: number) {
		const buf = _nbind.externalList[num] as
			number[] | ArrayBuffer | DataView | Uint8Array | Buffer;

		var NodeBuffer: typeof Buffer = Buffer;
		if(typeof(Buffer) != 'function') NodeBuffer = (function() {}) as any;

		if(buf instanceof Array) {
		} else {
			const src = HEAPU8.subarray(data, data + length);

			if(buf instanceof NodeBuffer) {
				let srcBuf: Buffer;

				if(typeof(Buffer.from) == 'function' && Buffer.from.length >= 3) {
					srcBuf = Buffer.from(src);
				} else srcBuf = new Buffer(src);

				srcBuf.copy(buf);
			} else (getBuffer(buf) as Uint8Array).set(src);
		}
	}
	/* tslint:enable */

	@prepareNamespace('_nbind')
	export class _ {} // tslint:disable-line:class-name
}
