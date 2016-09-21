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
import {typeModule, TypeFlags, TypeFlagBase, TypeSpec} from '../Type';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

const _removeAccessorPrefix = removeAccessorPrefix;

// tslint:disable-next-line:no-unused-variable
const _typeModule = typeModule;

export namespace _nbind {
	export var Pool: typeof _globals.Pool;
	export var typeList: typeof _globals.typeList;
	export var bigEndian: typeof _globals.bigEndian;

	export var addMethod: typeof _globals.addMethod;
	export var readTypeIdList: typeof _globals.readTypeIdList;
	export var readAsciiString: typeof _globals.readAsciiString;
	export var readPolicyList: typeof _globals.readPolicyList;
	export var makeTypeTbl: typeof _globals.makeTypeTbl;
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
	export var makeBound: typeof _class.makeBound;
	export var disableMember: typeof _class.disableMember;

	export var CallbackType: typeof _callback.CallbackType;

	export var CreateValueType: typeof _value.CreateValueType;
	export var Int64Type: typeof _value.Int64Type;
	export var popValue: typeof _value.popValue;

	export var ArrayType: typeof _std.ArrayType;
	export var StringType: typeof _std.StringType;

	export var makeCaller: typeof _caller.makeCaller;
	export var makeMethodCaller: typeof _caller.makeMethodCaller;

	export var BufferType: typeof _buffer.BufferType;

	export var enableLightGC: typeof _gc.enableLightGC;
	export var disableLightGC: typeof _gc.disableLightGC;
}

publishNamespace('_nbind');

// Ensure the __extends function gets defined.
class Dummy extends Boolean {}

@exportLibrary
class nbind { // tslint:disable-line:class-name

	@dep('_nbind')
	static _nbind_register_endian(byte: number) {
		if(byte == 1) _nbind.bigEndian = true;
	}

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
	}

	@dep('_nbind', '_typeModule')
	static _nbind_register_type(id: number, namePtr: number) {
		const name = _nbind.readAsciiString(namePtr);
		type TypeConstructor = { new(spec: TypeSpec): _type.BindType };
		const constructorTbl: { [name: string]: TypeConstructor } = {
			'bool': _nbind.BooleanType,
			'cbFunction &': _nbind.CallbackType,
			'std::string': _nbind.StringType,
			'Buffer': _nbind.BufferType,
			'Int64': _nbind.Int64Type,
			'_nbind_new': _nbind.CreateValueType
		};

		const constructor = constructorTbl[name] || _nbind.BindType;

		// tslint:disable-next-line:no-unused-expression
		new constructor({flags: TypeFlags.none, id: id, name: name});
	}

	@dep('_nbind')
	static _nbind_register_primitive(id: number, size: number, flag: number) {
		const spec = {
			flags: TypeFlags.isPrimitive | flag * TypeFlagBase.num,
			id: id,
			ptrSize: size
		};

		if(!_nbind.makeTypeTbl) {
			_nbind.makeTypeTbl = {
				[TypeFlags.isPrimitive]: _nbind.PrimitiveType,
				[TypeFlags.isBig]: _nbind.Int64Type,
				[TypeFlags.isClass]: _nbind.BindClass,
				[TypeFlags.isClassPtr]: _nbind.BindClassPtr,
				[TypeFlags.isSharedClassPtr]: _nbind.SharedClassPtr,
				[TypeFlags.isVector]: _nbind.ArrayType,
				[TypeFlags.isArray]: _nbind.ArrayType,
				[TypeFlags.isCString]: _nbind.CStringType,
				[TypeFlags.isOther]: _nbind.BindType
			};
		}

		_nbind.makeType(_nbind.makeTypeTbl, spec);
	}

	@dep('_nbind', '__extends')
	static _nbind_register_class(
		idListPtr: number,
		policyListPtr: number,
		namePtr: number,
		destructorPtr: number
	) {
		const name = _nbind.readAsciiString(namePtr);
		const policyTbl = _nbind.readPolicyList(policyListPtr);
		const idList = HEAPU32.subarray(idListPtr / 4, idListPtr / 4 + 2);

		const Bound = _nbind.makeBound(policyTbl);
		const flags = TypeFlags.isClass | (policyTbl['Value'] ? TypeFlags.isValueObject : 0);
		const id = idList[0];

		const bindClass = new _nbind.BindClass(
			{
				flags: flags,
				id: id,
				name: name
			},
			Bound
		);

		bindClass.ptrType = _nbind.getComplexType(
			idList[1],
			_nbind.makeTypeTbl,
			_nbind.getType,
			_nbind.queryType
		) as _class.BindClassPtr;

		const destroy = _nbind.makeMethodCaller(
			destructorPtr,
			0, // num
			TypeFlags.none,
			bindClass.name + '.free',
			id,
			// void destroy(uint32_t, void *ptr, void *shared, TypeFlags flags)
			['void', 'uint32_t', 'uint32_t'],
			null
		);

		_nbind.addMethod(
			bindClass.proto.prototype,
			'free',
			function() {
				destroy.call(this, this.__nbindShared, this.__nbindFlags);

				this.__nbindFlags |= TypeFlags.isDeleted;

				_nbind.disableMember(this, '__nbindShared');
				_nbind.disableMember(this, '__nbindPtr');
			},
			0
		);

		// Export the class.
		Module[name] = Bound;
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
		const bindClass = _nbind.typeList[typeID] as _class.BindClass;
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
		let bindClass: _class.BindClass;
		let target: any;

		if(typeID) {
			bindClass = _nbind.typeList[typeID] as _class.BindClass;
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
		const bindClass = _nbind.typeList[typeID] as _class.BindClass;
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

	@dep('_nbind')
	static nbind_enable_gc() { _nbind.enableLightGC(); }

	@dep('_nbind')
	static nbind_disable_gc() { _nbind.disableLightGC(); }

}
