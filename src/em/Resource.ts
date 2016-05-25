// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import {setEvil, prepareNamespace} from 'emscripten-library-decorator';
import {_nbind as _type} from './BindingType';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {

	export class Resource {
		constructor(open: string, close: string) {
			this.open = open;
			this.close = close;
		}

		open: string;

		close: string;
	}

	/** Create a single resource with open and close code included
	  * once from each type of resource needed by a list of types. */

	export function listResources(readList: _type.BindType[], writeList: _type.BindType[]) {
		var openTbl: { [name: string]: boolean } = {};
		var closeTbl: { [name: string]: boolean } = {};

		for(var bindType of readList) {
			for(var resource of bindType.readResources || []) {
				openTbl[resource.open] = true;
				closeTbl[resource.close] = true;
			}
		}

		for(var bindType of writeList) {
			for(var resource of bindType.writeResources || []) {
				openTbl[resource.open] = true;
				closeTbl[resource.close] = true;
			}
		}

		return(new Resource(
			Object.keys(openTbl).join(''),
			Object.keys(closeTbl).join('')
		));
	}

	export var resources = {
		stack: new Resource(
			'var sp=Runtime.stackSave();',
			'Runtime.stackRestore(sp);'
		),

		pool: new Resource(
			'',
			'' // TODO: call lreset()
		)
	};

	@prepareNamespace('_nbind')
	export class _ {}
}
