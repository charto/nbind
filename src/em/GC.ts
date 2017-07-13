// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles creating invoker functions for Emscripten dyncalls
// wrapped in type conversions for arguments and return values.

import { setEvil, prepareNamespace } from 'emscripten-library-decorator';
import { _nbind as _wrapper } from './Wrapper';
import { StateFlags } from '../Type';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {

	type Wrapper = _wrapper.Wrapper;

	let dirtyList: Wrapper[] = [];

	let gcTimer: number | NodeJS.Timer = 0;

	function sweep() {
		for(let obj of dirtyList) {
			if(!(obj.__nbindState & (StateFlags.isPersistent | StateFlags.isDeleted))) {
				obj.free!();
			}
		}

		dirtyList = [];
		gcTimer = 0;
	}

	// tslint:disable-next-line:no-empty
	export let mark = (obj: Wrapper) => {};

	export function toggleLightGC(enable: boolean) {
		if(enable) {
			mark = (obj: Wrapper) => {
				dirtyList.push(obj);

				if(!gcTimer) gcTimer = setTimeout(sweep, 0);
			};
		} else {
			// tslint:disable-next-line:no-empty
			mark = (obj: Wrapper) => {};
		}
	}

	@prepareNamespace('_nbind')
	export class _ {} // tslint:disable-line:class-name
}
