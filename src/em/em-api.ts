// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file contains JavaScript functions directly exposed to C++ through
// Emscripten library exports.

import {
	setEvil,
	publishNamespace,
	exportLibrary,
	dep
} from 'emscripten-library-decorator';

import { _nbind as _globals } from './Globals';   export { _globals };
import { _nbind as _type } from './BindingType';  export { _type };
import { _nbind as _class } from './BindClass';   export { _class };
import { _nbind as _external } from './External'; export { _external };
import { _nbind as _callback } from './Callback'; export { _callback };
import { _nbind as _value } from './ValueObj';    export { _value };
import { _nbind as _std } from './BindingStd';    export { _std };
import { _nbind as _caller } from './Caller';     export { _caller };
import { _nbind as _wrapper } from './Wrapper';   export { _wrapper };
import { _nbind as _resource } from './Resource'; export { _resource };
import { _nbind as _buffer } from './Buffer';     export { _buffer };
import { _nbind as _gc } from './GC';             export { _gc };
import { SignatureType, removeAccessorPrefix } from '../common';
import { typeModule, TypeFlags, TypeSpecWithName } from '../Type';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

const _removeAccessorPrefix = removeAccessorPrefix;
const _typeModule = typeModule;

export namespace _nbind {
	export let Pool: typeof _globals.Pool;
	export let bigEndian: typeof _globals.bigEndian;

	export let readTypeIdList: typeof _globals.readTypeIdList;
	export let readAsciiString: typeof _globals.readAsciiString;
	export let readPolicyList: typeof _globals.readPolicyList;
	export let makeTypeKindTbl: typeof _globals.makeTypeKindTbl;
	export let makeTypeNameTbl: typeof _globals.makeTypeNameTbl;
	export let constructType: typeof _globals.constructType;
	export let getType: typeof _globals.getType;
	export let queryType: typeof _globals.queryType;

	export let makeType: typeof _type.makeType;
	export let getComplexType: typeof _type.getComplexType;
	export let BindType: typeof _type.BindType;
	export let PrimitiveType: typeof _type.PrimitiveType;
	export let BooleanType: typeof _type.BooleanType;
	export let CStringType: typeof _type.CStringType;

	export let BindClass: typeof _class.BindClass;
	export let BindClassPtr: typeof _class.BindClassPtr;
	export let SharedClassPtr: typeof _class.BindClassPtr;
	export let callUpcast: typeof _class.callUpcast;

	export let ExternalType: typeof _external.ExternalType;

	export let CallbackType: typeof _callback.CallbackType;

	export let CreateValueType: typeof _value.CreateValueType;
	export let Int64Type: typeof _value.Int64Type;
	export let popValue: typeof _value.popValue;

	export let ArrayType: typeof _std.ArrayType;
	export let StringType: typeof _std.StringType;

	export let makeMethodCaller: typeof _caller.makeMethodCaller;

	export let BufferType: typeof _buffer.BufferType;

	export let toggleLightGC: typeof _gc.toggleLightGC;
}

publishNamespace('_nbind');

// Ensure the __extends function gets defined.
class Dummy extends Boolean {}

@exportLibrary
class nbind { // tslint:disable-line:class-name

	@dep('_nbind')
	static _nbind_register_pool(
		pageSize: number,
		usedPtr: number,
		rootPtr: number,
		pagePtr: number
	) {
		_nbind.Pool.pageSize = pageSize;
		_nbind.Pool.usedPtr = usedPtr / 4;
		_nbind.Pool.rootPtr = rootPtr;
		_nbind.Pool.pagePtr = pagePtr / 4;

		HEAP32[usedPtr / 4] = 0x01020304;
		if(HEAP8[usedPtr] == 1) _nbind.bigEndian = true;
		HEAP32[usedPtr / 4] = 0;

		_nbind.makeTypeKindTbl = {
			[TypeFlags.isArithmetic]: _nbind.PrimitiveType,
			[TypeFlags.isBig]: _nbind.Int64Type,
			[TypeFlags.isClass]: _nbind.BindClass,
			[TypeFlags.isClassPtr]: _nbind.BindClassPtr,
			[TypeFlags.isSharedClassPtr]: _nbind.SharedClassPtr,
			[TypeFlags.isVector]: _nbind.ArrayType,
			[TypeFlags.isArray]: _nbind.ArrayType,
			[TypeFlags.isCString]: _nbind.CStringType,
			[TypeFlags.isCallback]: _nbind.CallbackType,
			[TypeFlags.isOther]: _nbind.BindType
		};

		_nbind.makeTypeNameTbl = {
			'Buffer': _nbind.BufferType,
			'External': _nbind.ExternalType,
			'Int64': _nbind.Int64Type,
			'_nbind_new': _nbind.CreateValueType,
			'bool': _nbind.BooleanType,
			// 'cbFunction': _nbind.CallbackType,
			'cbFunction &': _nbind.CallbackType,
			'const cbFunction &': _nbind.CallbackType,
			'const std::string &': _nbind.StringType,
			'std::string': _nbind.StringType
		};

		Module['toggleLightGC'] = _nbind.toggleLightGC;
		_nbind.callUpcast = Module['dynCall_ii'];

		const globalScope = _nbind.makeType(_nbind.constructType, {
			flags: TypeFlags.isClass,
			id: 0,
			name: ''
		}) as _class.BindClass;

		globalScope.proto = Module as any;
		_nbind.BindClass.list.push(globalScope);
	}

	@dep('_nbind', '_typeModule')
	static _nbind_register_type(id: number, namePtr: number) {
		const name = _nbind.readAsciiString(namePtr);

		const spec = {
			flags: TypeFlags.isOther,
			id: id,
			name: name
		};

		_nbind.makeType(_nbind.constructType, spec);
	}

	@dep('_nbind')
	static _nbind_register_primitive(id: number, size: number, flags: number) {
		const spec = {
			flags: TypeFlags.isArithmetic | flags,
			id: id,
			ptrSize: size
		};

		_nbind.makeType(_nbind.constructType, spec);
	}

	@dep('_nbind', '__extends')
	static _nbind_register_class(
		idListPtr: number,
		policyListPtr: number,
		superListPtr: number,
		upcastListPtr: number,
		superCount: number,
		destructorPtr: number,
		namePtr: number
	) {
		const name = _nbind.readAsciiString(namePtr);
		const policyTbl = _nbind.readPolicyList(policyListPtr);
		const idList = HEAPU32.subarray(idListPtr / 4, idListPtr / 4 + 2);

		const spec: TypeSpecWithName = {
			flags: TypeFlags.isClass | (policyTbl['Value'] ? TypeFlags.isValueObject : 0),
			id: idList[0],
			name: name
		};

		const bindClass = _nbind.makeType(_nbind.constructType, spec) as _class.BindClass;

		bindClass.ptrType = _nbind.getComplexType(
			idList[1],
			_nbind.constructType,
			_nbind.getType,
			_nbind.queryType
		) as _class.BindClassPtr;

		bindClass.destroy = _nbind.makeMethodCaller(bindClass.ptrType, {
			boundID: spec.id,
			flags: TypeFlags.none,
			name: 'destroy',
			num: 0,
			ptr: destructorPtr,
			title: bindClass.name + '.free',
			typeList: ['void', 'uint32_t', 'uint32_t']
		}) as (shared: number, flags: number) => void;

		if(superCount) {
			bindClass.superIdList = Array.prototype.slice.call(
				HEAPU32.subarray(superListPtr / 4, superListPtr / 4 + superCount)
			);
			bindClass.upcastList = Array.prototype.slice.call(
				HEAPU32.subarray(upcastListPtr / 4, upcastListPtr / 4 + superCount)
			);
		}

		Module[bindClass.name] = bindClass.makeBound(policyTbl);

		_nbind.BindClass.list.push(bindClass);
	}

	@dep('_nbind', '_removeAccessorPrefix')
	static _nbind_register_function(
		boundID: number,
		policyListPtr: number,
		typeListPtr: number,
		typeCount: number,
		ptr: number,
		direct: number,
		signatureType: SignatureType,
		namePtr: number,
		num: number,
		flags: TypeFlags
	) {
		const bindClass = _nbind.getType(boundID) as _class.BindClass;
		const policyTbl = _nbind.readPolicyList(policyListPtr);
		const typeList = _nbind.readTypeIdList(typeListPtr, typeCount);

		let specList: _class.MethodSpec[];

		if(signatureType == SignatureType.construct) {
			specList = [{
				direct: ptr,
				name: '__nbindConstructor',
				ptr: 0,
				title: bindClass.name + ' constructor',
				typeList: ['uint32_t'].concat(typeList.slice(1))
			}, {
				direct: direct,
				name: '__nbindValueConstructor',
				ptr: 0,
				title: bindClass.name + ' value constructor',
				typeList: ['void', 'uint32_t'].concat(typeList.slice(1))
			}];
		} else {
			let name = _nbind.readAsciiString(namePtr);
			const title = (bindClass.name && bindClass.name + '.') + name;

			if(signatureType == SignatureType.getter || signatureType == SignatureType.setter) {
				name = _removeAccessorPrefix(name);
			}

			specList = [{
				boundID: boundID,
				direct: direct,
				name: name,
				ptr: ptr,
				title: title,
				typeList: typeList
			}];
		}

		for(let spec of specList) {
			spec.signatureType = signatureType;
			spec.policyTbl = policyTbl;
			spec.num = num;
			spec.flags = flags;

			bindClass.addMethod(spec);
		}
	}

	@dep('_nbind')
	static _nbind_finish() {
		for(let bindClass of _nbind.BindClass.list) bindClass.finish();
	}

	@dep('_nbind')
	static nbind_debug() { debugger; }

}
