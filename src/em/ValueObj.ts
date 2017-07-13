// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles value objects, which are represented by equivalent C++ and
// JavaScript classes, with toJS and fromJS methods calling each others'
// constructors to marshal the class between languages and providing a similar
// API in both.

import {
	setEvil,
	prepareNamespace,
	defineHidden,
	exportLibrary,
	dep
} from 'emscripten-library-decorator';
import { _nbind as _globals } from './Globals';
import { _nbind as _type } from './BindingType';
import { _nbind as _class } from './BindClass';
import { _nbind as _wrapper } from './Wrapper';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

const _defineHidden = defineHidden;

export namespace _nbind {
	export const BindType = _type.BindType;
}

export namespace _nbind {

	export let popShared: typeof _class.popShared;
	type BindClassPtr = _class.BindClassPtr;

	export let throwError: typeof _globals.throwError;
	export let typeNameTbl: typeof _globals.typeNameTbl;
	export let bigEndian: typeof _globals.bigEndian;

	export interface ValueObject {
		fromJS(output: (...args: any[]) => void): void;

		/** This is dynamically created inside nbind. */
		__nbindValueConstructor: _globals.Func;
	}

	/** Storage for value objects. Slot 0 is reserved to represent errors. */
	export const valueList: (ValueObject | number)[] = [ 0 ];

	/** Value object storage slot free list head. */
	let firstFreeValue = 0;

	export function pushValue(value: ValueObject) {
		let num = firstFreeValue;

		if(num) {
			firstFreeValue = valueList[num] as number;
		} else num = valueList.length;

		valueList[num] = value;
		return(num * 2 + 1);
	}

	export function popValue(
		num: number,
		type?: BindClassPtr
	): ValueObject | _wrapper.Wrapper | null {
		if(!num) throwError('Value type JavaScript class is missing or not registered');

		if(num & 1) {
			num >>= 1;
			const obj = valueList[num] as ValueObject;

			valueList[num] = firstFreeValue;
			firstFreeValue = num;

			return(obj);
		} else if(type) {
			return(popShared(num, type));
		} else throw(new Error('Invalid value slot ' + num));

	}

	// 2^64, first integer not representable with uint64_t.
	// Start of range used for other flags.
	const valueBase = 18446744073709551616.0;

	function push64(num: number | any) {
		if(typeof(num) == 'number') return(num);

		return(pushValue(num) * 4096 + valueBase);
	}

	function pop64(num: number): number | any {
		if(num < valueBase) return(num);
		return(popValue((num - valueBase) / 4096));
	}

	// Special type that constructs a new object.

	export class CreateValueType extends BindType {
		makeWireWrite(expr: string) {
			return('(_nbind.pushValue(new ' + expr + '))');
		}
	}

	export class Int64Type extends BindType {
		wireWrite = push64;
		wireRead = pop64;
	}

	@prepareNamespace('_nbind')
	export class _ {} // tslint:disable-line:class-name
}

@exportLibrary
class nbind { // tslint:disable-line:class-name

	// Initialize a C++ object based on a JavaScript object's contents.

	@dep('_nbind')
	static _nbind_get_value_object(num: number, ptr: number) {
		const obj = _nbind.popValue(num) as _nbind.ValueObject;

		if(!obj.fromJS) {
			throw(new Error('Object ' + obj + ' has no fromJS function'));
		}

		obj.fromJS(function(this: _nbind.ValueObject) {
			obj.__nbindValueConstructor.apply(
				this,
				Array.prototype.concat.apply([ptr], arguments)
			);
		});
	}

	@dep('_nbind')
	static _nbind_get_int_64(num: number, ptr: number) {
		const obj = _nbind.popValue(num) as _nbind.ValueObject;

		obj.fromJS(function(lo: number, hi: number, sign: boolean) {
			if(sign) {
				lo = ~lo;
				hi = ~hi;

				if(!++lo) ++hi;
			}

			ptr >>= 2;

			if(_nbind.bigEndian) {
				// Emscripten itself might not work on big endian,
				// but we support it here anyway.
				HEAP32[ptr] = hi;
				HEAP32[ptr + 1] = lo;
			} else {
				HEAP32[ptr] = lo;
				HEAP32[ptr + 1] = hi;
			}
		});
	}

	@dep('_nbind')
	static nbind_value(name: string, proto: any) {
		if(!_nbind.typeNameTbl[name]) _nbind.throwError('Unknown value type ' + name);
		Module['NBind'].bind_value(name, proto);

		// Copy value constructor reference from C++ wrapper prototype
		// to equivalent JS prototype.

		_defineHidden(
			(_nbind.typeNameTbl[name] as _class.BindClass).proto.prototype.__nbindValueConstructor
		)(proto.prototype, '__nbindValueConstructor');
	}

}
