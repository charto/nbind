// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// These must match C++ enum SignatureType in BaseSignature.h

import {setEvil, prepareNamespace} from 'emscripten-library-decorator';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {

	export enum SignatureType {
		func = 0,
		method,
		getter,
		setter,
		construct
	}

	// These must match C++ enum StructureType in TypeID.h

	export enum StructureType {
		raw = 0,
		vector,
		array
	}

	@prepareNamespace('_nbind')
	export class _ {} // tslint:disable-line:class-name

}
