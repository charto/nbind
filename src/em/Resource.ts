// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles resource allocation and freeing for invoker functions.
// For example if any type conversion requires space in the C++ stack,
// at the end of the invoker it must be reset as it was before.

import { setEvil, prepareNamespace } from 'emscripten-library-decorator';
import { _nbind as _type } from './BindingType';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {

	export class Resource {
		constructor(open?: string, close?: string) {
			if(open) this.openTbl[open] = true;
			if(close) this.closeTbl[close] = true;
		}

		add(other: Resource) {
			for(let key of Object.keys(other.openTbl)) this.openTbl[key] = true;
			for(let key of Object.keys(other.closeTbl)) this.closeTbl[key] = true;
		}

		remove(other: Resource) {
			for(let key of Object.keys(other.openTbl)) delete(this.openTbl[key]);
			for(let key of Object.keys(other.closeTbl)) delete(this.closeTbl[key]);
		}

		makeOpen = () => Object.keys(this.openTbl).join('');
		makeClose = () => Object.keys(this.closeTbl).join('');

		openTbl: { [name: string]: boolean } = {};
		closeTbl: { [name: string]: boolean } = {};
	}

	/** Create a single resource with open and close code included
	  * once from each type of resource needed by a list of types. */

	export function listResources(readList: _type.BindType[], writeList: _type.BindType[]) {
		const result = new Resource();

		for(let bindType of readList) {
			for(let resource of bindType.readResources || []) {
				result.add(resource);
			}
		}

		for(let bindType of writeList) {
			for(let resource of bindType.writeResources || []) {
				result.add(resource);
			}
		}

		return(result);
	}

	export const resources = {
		pool: new Resource(
			'var used=HEAPU32[_nbind.Pool.usedPtr],page=HEAPU32[_nbind.Pool.pagePtr];',
			'_nbind.Pool.lreset(used,page);'
		)
/*
		stack: new Resource(
			'var sp=Runtime.stackSave();',
			'Runtime.stackRestore(sp);'
		)
*/
	};

	@prepareNamespace('_nbind')
	export class _ {} // tslint:disable-line:class-name
}
