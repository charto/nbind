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
import {_nbind as _globals} from './Globals';
import {_nbind as _type} from './BindingType';
import {_nbind as _class} from './BindClass';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

const _defineHidden = defineHidden;

export namespace _nbind {
	export var BindType = _type.BindType;
}

export namespace _nbind {

	export var throwError: typeof _globals.throwError;
	export var typeTbl: typeof _globals.typeTbl;
	export var bigEndian: typeof _globals.bigEndian;

	export interface ValueObject {
		fromJS(output: (...args: any[]) => void): void;

		/** This is mandatory, but dynamically created inside nbind. */
		__nbindValueConstructor?: _globals.Func;
	}

	/** Storage for value objects. Slot 0 is reserved to represent errors. */
	export var valueList: ValueObject[] = [ null ];

	/** List of free slots in value object storage. */
	export var valueFreeList: number[] = [];

	export function pushValue(value: ValueObject) {
		const num = valueFreeList.pop() || valueList.length;

		valueList[num] = value;
		return(num);
	}

	export function popValue(num: number) {
		if(!num) throwError('Value type JavaScript class is missing or not registered');

		const obj = valueList[num];

		valueList[num] = null;
		valueFreeList.push(num);
		return(obj);
	}

	// 2^64, first integer not representable with uint64_t.
	// Start of range used for other flags.
	const valueBase = 18446744073709551616.0;

	export function push64(num: number | any) {
		if(typeof(num) == 'number') return(num);

		const wrapNum = valueFreeList.pop() || valueList.length;

		valueList[wrapNum] = num;
		return(wrapNum * 4096 + valueBase);
	}

	export function pop64(num: number): number | any {
		if(num < valueBase) return(num);
		return(popValue((num - valueBase) / 4096));
	}

	// Special type that constructs a new object.

	export class CreateValueType extends BindType {
		constructor(id: number, name: string) {
			super(id, name);
		}

		makeWireWrite = (expr: string) => '(_nbind.pushValue(new ' + expr + '))';
	}

	export class Int64Type extends BindType {
		constructor(id: number, name: string) {
			super(id, name);
		}

		wireWrite = push64;
		wireRead = pop64;
	}

	@prepareNamespace('_nbind')
	export class _ {} // tslint:disable-line:class-name
}

@exportLibrary
class nbind { // tslint:disable-line:class-name

	@dep('_nbind')
	static _nbind_get_value_object(num: number, ptr: number) {
		const obj = _nbind.popValue(num);

		obj.fromJS(function() {
			obj.__nbindValueConstructor.apply(
				this,
				Array.prototype.concat.apply([ptr], arguments)
			);
		});
	}

	@dep('_nbind')
	static _nbind_get_int_64(num: number, ptr: number) {
		const obj = _nbind.popValue(num);

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
		Module['NBind'].bind_value(name, proto);

		// Copy value constructor reference from C++ wrapper prototype
		// to equivalent JS prototype.

		_defineHidden(
			(_nbind.typeTbl[name] as _class.BindClass).proto.prototype.__nbindValueConstructor
		)(proto.prototype, '__nbindValueConstructor');
	}

}
