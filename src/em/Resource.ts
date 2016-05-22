// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import {setEvil, prepareNamespace} from 'emscripten-library-decorator';
import {_nbind as main} from './nbind-em';

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

	export function listResources(typeList: main.BindType[]) {
		var openTbl: { [name: string]: boolean } = {};
		var closeTbl: { [name: string]: boolean } = {};

		for(var bindType of typeList) {
			if(!bindType.needsResources) continue;

			for(var resource of bindType.needsResources) {
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
		)
	};

	@prepareNamespace('_nbind')
	export class _ {}
}
