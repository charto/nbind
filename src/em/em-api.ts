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
import { _nbind as _resource } from './Resource'; export { _resource };
import { _nbind as _buffer } from './Buffer';     export { _buffer };
import { _nbind as _gc } from './GC';             export { _gc };
import {SignatureType, removeAccessorPrefix} from '../common';
import {typeModule, TypeFlags, TypeSpecWithName} from '../Type';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

const _removeAccessorPrefix = removeAccessorPrefix;

// tslint:disable-next-line:no-unused-variable
const _typeModule = typeModule;

export namespace _nbind {
	export var Pool: typeof _globals.Pool;
	export var bigEndian: typeof _globals.bigEndian;

	export var addMethod: typeof _globals.addMethod;
	export var readTypeIdList: typeof _globals.readTypeIdList;
	export var readAsciiString: typeof _globals.readAsciiString;
	export var readPolicyList: typeof _globals.readPolicyList;
	export var makeTypeKindTbl: typeof _globals.makeTypeKindTbl;
	export var makeTypeNameTbl: typeof _globals.makeTypeNameTbl;
	export var constructType: typeof _globals.constructType;
	export var getType: typeof _globals.getType;
	export var queryType: typeof _globals.queryType;

	export var makeType: typeof _type.makeType;
	export var getComplexType: typeof _type.getComplexType;
	export var BindType: typeof _type.BindType;
	export var PrimitiveType: typeof _type.PrimitiveType;
	export var BooleanType: typeof _type.BooleanType;
	export var CStringType: typeof _type.CStringType;

	export var BindClass: typeof _class.BindClass;
	export var BindClassPtr: typeof _class.BindClassPtr;
	export var SharedClassPtr: typeof _class.BindClassPtr;
	export var pendingChildTbl: typeof _class.pendingChildTbl;

	export var ExternalType: typeof _external.ExternalType;

	export var CallbackType: typeof _callback.CallbackType;

	export var CreateValueType: typeof _value.CreateValueType;
	export var Int64Type: typeof _value.Int64Type;
	export var popValue: typeof _value.popValue;

	export var ArrayType: typeof _std.ArrayType;
	export var StringType: typeof _std.StringType;

	export var makeCaller: typeof _caller.makeCaller;
	export var makeMethodCaller: typeof _caller.makeMethodCaller;

	export var BufferType: typeof _buffer.BufferType;

	export var toggleLightGC: typeof _gc.toggleLightGC;
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
			[TypeFlags.isOther]: _nbind.BindType
		};

		_nbind.makeTypeNameTbl = {
			'bool': _nbind.BooleanType,
			'cbFunction &': _nbind.CallbackType,
			'std::string': _nbind.StringType,
			'External': _nbind.ExternalType,
			'Buffer': _nbind.BufferType,
			'Int64': _nbind.Int64Type,
			'_nbind_new': _nbind.CreateValueType
		};

		Module['toggleLightGC'] = _nbind.toggleLightGC;
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
		superCount: number,
		namePtr: number,
		destructorPtr: number
	) {
		const name = _nbind.readAsciiString(namePtr);
		const policyTbl = _nbind.readPolicyList(policyListPtr);
		const idList = HEAPU32.subarray(idListPtr / 4, idListPtr / 4 + 2);
		const superIdList = HEAPU32.subarray(superListPtr / 4, superListPtr / 4 + superCount);

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

		bindClass.destroy = _nbind.makeMethodCaller(
			destructorPtr,
			0, // num
			TypeFlags.none,
			bindClass.name + '.free',
			spec.id,
			['void', 'uint32_t', 'uint32_t'],
			null
		) as (shared: number, flags: number) => void;

		const childTbl = _nbind.pendingChildTbl;

		Array.prototype.forEach.call(superIdList, (superId: number) => {
			const superClass = _nbind.getType(superId) as _class.BindClass;

			if(superClass) {
				bindClass.superList.push(superClass);
			} else {
				const childList = childTbl[superId] || [];
				childList.push(idList[0]);
				childTbl[superId] = childList;

				++bindClass.pendingSuperCount;
			}
		});

		// tslint:disable-next-line:no-shadowed-variable
		function register(bindClass: _class.BindClass, id: number) {
			if(!bindClass.pendingSuperCount) {
				// Export the class.
				Module[bindClass.name] = bindClass.makeBound(policyTbl);

				// Try to export child classes.
				for(let childId of childTbl[id] || []) {
					const childClass = _nbind.getType(childId) as _class.BindClass;

					childClass.superList.push(bindClass);

					--childClass.pendingSuperCount;
					register(childClass, childId);
				}
			}
		}

		register(bindClass, idList[0]);
	}

	@dep('_nbind')
	static _nbind_register_constructor(
		typeID: number,
		policyListPtr: number,
		typeListPtr: number,
		typeCount: number,
		ptr: number,
		ptrValue: number
	) {
		const policyTbl = _nbind.readPolicyList(policyListPtr);
		const typeList = _nbind.readTypeIdList(typeListPtr, typeCount);
		const bindClass = _nbind.getType(typeID) as _class.BindClass;
		const proto = bindClass.proto.prototype;

		// The constructor returns a pointer to the new object.
		// It fits in uint32_t.

		typeList[0] = 'uint32_t';

		_nbind.addMethod(
			proto,
			'__nbindConstructor',
			_nbind.makeCaller(
				null,
				0, // num
				TypeFlags.none,
				bindClass.name + ' constructor',
				ptr,
				typeList,
				policyTbl
			),
			typeCount - 1
		);

		// First argument is a pointer to the C++ object to construct in place.
		// It fits in uint32_t.

		typeList.splice(0, 1, 'void', 'uint32_t');

		_nbind.addMethod(
			proto,
			'__nbindValueConstructor',
			_nbind.makeCaller(
				null,
				0, // num
				TypeFlags.none,
				bindClass.name + ' value constructor',
				ptrValue,
				typeList,
				policyTbl
			),
			typeCount
		);
	}

	@dep('_nbind')
	static _nbind_register_function(
		typeID: number,
		policyListPtr: number,
		typeListPtr: number,
		typeCount: number,
		ptr: number,
		namePtr: number,
		num: number,
		flags: TypeFlags,
		direct: number
	) {
		const name = _nbind.readAsciiString(namePtr);
		const policyTbl = _nbind.readPolicyList(policyListPtr);
		const typeList = _nbind.readTypeIdList(typeListPtr, typeCount);
		let bindClass: _class.BindClass | undefined;
		let target: any;

		if(typeID) {
			bindClass = _nbind.getType(typeID) as _class.BindClass;
			target = bindClass.proto;
		} else {
			target = Module;
		}

		_nbind.addMethod(
			target,
			name,
			_nbind.makeCaller(
				ptr,
				num,
				flags,
				(bindClass ? bindClass.name + '.' : '') + name,
				direct,
				typeList,
				policyTbl
			),
			typeCount - 1
		);
	}

	@dep('_nbind', '_removeAccessorPrefix')
	static _nbind_register_method(
		typeID: number,
		policyListPtr: number,
		typeListPtr: number,
		typeCount: number,
		ptr: number,
		namePtr: number,
		num: number,
		flags: TypeFlags,
		signatureType: SignatureType
	) {
		let name = _nbind.readAsciiString(namePtr);
		const policyTbl = _nbind.readPolicyList(policyListPtr);
		const typeList = _nbind.readTypeIdList(typeListPtr, typeCount);
		const bindClass = _nbind.getType(typeID) as _class.BindClass;
		const proto = bindClass.proto.prototype;

		if(signatureType == SignatureType.method) {
			_nbind.addMethod(
				proto,
				name,
				_nbind.makeMethodCaller(
					ptr,
					num,
					flags,
					bindClass.name + '.' + name,
					typeID,
					typeList,
					policyTbl
				),
				typeCount - 1
			);

			return;
		}

		name = _removeAccessorPrefix(name);

		if(signatureType == SignatureType.setter) {

			// A setter is always followed by a getter, so we can just
			// temporarily store an invoker in the property.
			// The getter definition then binds it properly.

			proto[name] = _nbind.makeMethodCaller(
				ptr,
				num,
				flags,
				bindClass.name + '.' + 'set ' + name,
				typeID,
				typeList,
				policyTbl
			);
		} else {
			Object.defineProperty(proto, name, {
				configurable: true,
				enumerable: true,
				get: _nbind.makeMethodCaller(
					ptr,
					num,
					flags,
					bindClass.name + '.' + 'get ' + name,
					typeID,
					typeList,
					policyTbl
				) as () => any,
				set: proto[name]
			});
		}
	}

	@dep('_nbind')
	static nbind_debug() { debugger; }

}
