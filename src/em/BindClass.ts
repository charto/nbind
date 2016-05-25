// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import {setEvil, prepareNamespace} from 'emscripten-library-decorator';
import {_nbind as _globals} from './Globals';
import {_nbind as _type} from './BindingType';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {
	export var BindType = _type.BindType;
}

export namespace _nbind {

	// Base class for wrapped instances of bound C++ classes.

	export class Wrapper {
		free: () => void;

		/** Dynamically set by _nbind_register_constructor.
		  * Calls the C++ constructor and returns a numeric heap pointer. */
		__nbindConstructor: (...args: any[]) => number;
		__nbindValueConstructor: _globals.Func;
		/** __nbindConstructor return value. */
		__nbindPtr: number;
	}

	// Any subtype (not instance but type) of Wrapper.
	// Declared as anything that constructs something compatible with Wrapper.

	interface WrapperClass {
		new(...args: any[]): Wrapper;
	}

	// Base class for all bound C++ classes (not their instances),
	// also inheriting from a generic type definition.

	export class BindClass extends BindType {
		constructor(id: number, name: string, proto: WrapperClass) {
			super(id, name);

			this.proto = proto;
		}

		// Reference to JavaScript class for wrapped instances
		// of this C++ class.

		proto: WrapperClass;

		makeWireRead = (expr: string) => ('(' +
			expr + '||' +
			'_nbind.throwError("Value type JavaScript class is missing or not registered"),' +
			'_nbind.value' +
		')');
		makeWireWrite = (expr: string) => '_nbind.pushValue(' + expr + ')';
	}

	@prepareNamespace('_nbind')
	export class _ {}
}
