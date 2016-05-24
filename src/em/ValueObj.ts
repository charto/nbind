// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import {setEvil, prepareNamespace} from 'emscripten-library-decorator';
import {_nbind as _type} from './BindingType';

setEvil((code: string) => eval(code));

export namespace _nbind {
	export var BindType = _type.BindType;
}

export namespace _nbind {

	export type Func = (...args: any[]) => any;

	export interface ValueObject {
		fromJS(output: () => void): void;

		/** This is mandatory, but dynamically created inside nbind. */
		__nbindValueConstructor?: Func;
	}

	export var valueList: ValueObject[] = [];

	export var valueFreeList: number[] = [];

	export function pushValue(value: ValueObject) {
		var num = valueFreeList.pop() || valueList.length;

		valueList[num] = value;
		return(num);
	}

	export function popValue(num: number) {
		var obj = valueList[num];

		valueList[num] = null;
		valueFreeList.push(num);
		return(obj);
	}

	// Special type that constructs a new object.

	export class CreateValueType extends BindType {
		constructor(id: number, name: string) {
			super(id, name);
		}

		needsWireWrite: boolean = true;

		makeWireWrite(expr: string) {
			return('((_nbind.value=new ' + expr + '),0)');
		}
	}

	@prepareNamespace('_nbind')
	export class _ {}
}
