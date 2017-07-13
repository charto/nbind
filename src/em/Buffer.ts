// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import { setEvil, prepareNamespace } from 'emscripten-library-decorator';
import { _nbind as _globals } from './Globals';
import { _nbind as _type } from './BindingType';
import { _nbind as _external } from './External';
import { _nbind as _resource } from './Resource';
import { PolicyTbl } from '../Type';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {
	export const Pool = _globals.Pool;
	export const BindType = _type.BindType;
	export const External = _external.External;
}

export namespace _nbind {

	export let externalList: typeof _external.externalList;

	export let resources: typeof _resource.resources;

	class ExternalBuffer extends External<
		number[] | ArrayBuffer | DataView | Uint8Array | Buffer
	> {
		constructor(buf: any, ptr: number) {
			super(buf);
			this.ptr = ptr;
		}

		free() { _free(this.ptr); }

		ptr: number;
	}

	function getBuffer(
		buf: number[] | ArrayBuffer | DataView | Uint8Array | Buffer
	): number[] | Uint8Array | Buffer {
		if(buf instanceof ArrayBuffer) {
			return(new Uint8Array(buf));
		} else if(buf instanceof DataView) {
			return(new Uint8Array(buf.buffer, buf.byteOffset, buf.byteLength));
		} else return(buf);
	}

	function pushBuffer(
		buf: number[] | ArrayBuffer | DataView | Uint8Array | Buffer,
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
		HEAPU32[ptr++] = new ExternalBuffer(buf, data).register();

		HEAPU8.set(getBuffer(buf), data);

		return(result);
	}

	export class BufferType extends BindType {
		makeWireWrite(expr: string, policyTbl: PolicyTbl) {
			return((arg: any) => pushBuffer(arg, policyTbl));
		}

		wireWrite = pushBuffer;

		readResources = [ resources.pool ];
		writeResources = [ resources.pool ];
	}

	// Called from EM_ASM block in Buffer.h

	export function commitBuffer(num: number, data: number, length: number) {
		const buf = (_nbind.externalList[num] as ExternalBuffer).data;

		let NodeBuffer: typeof Buffer = Buffer;
		// tslint:disable-next-line:no-empty
		if(typeof(Buffer) != 'function') NodeBuffer = (function() {}) as any;

		if(buf instanceof Array) {
			// TODO if needed
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

	@prepareNamespace('_nbind')
	export class _ {} // tslint:disable-line:class-name
}
