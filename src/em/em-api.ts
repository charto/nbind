// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import {setEvil, publishNamespace, defineHidden, exportLibrary, dep} from 'emscripten-library-decorator';
import {_nbind as _globals} from './Globals';
import {_nbind as _type} from './BindingType';
import {_nbind as _class} from './BindClass';
import {_nbind as _callback} from './Callback';
import {_nbind as _value} from './ValueObj';
import {_nbind as _std} from './BindingStd';
import {_nbind as _caller} from './Caller';
import {_nbind as _resource} from './Resource';

export {_globals, _type, _class, _callback, _value, _std, _caller, _resource};

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

var _defineHidden = defineHidden;

export namespace _nbind {
	export type Func = _globals.Func;

	export var MethodType: typeof _globals.MethodType;
	export var addMethod: typeof _globals.addMethod;

	export var typeList: typeof _globals.typeList;

	export var BindType: typeof _type.BindType;
	export var PrimitiveType: typeof _type.PrimitiveType;
	export var BooleanType: typeof _type.BooleanType;
	export var CStringType: typeof _type.CStringType;

	export var Wrapper: typeof _class.Wrapper;
	export var BindClass: typeof _class.BindClass;

	export var CallbackType: typeof _callback.CallbackType;
	export var unregisterCallback: typeof _callback.unregisterCallback;

	export var callbackRefCountList: typeof _callback.callbackRefCountList;
	export var callbackSignatureList: typeof _callback.callbackSignatureList;

	export var CreateValueType: typeof _value.CreateValueType;
	export var popValue: typeof _value.popValue;

	export var StringType: typeof _std.StringType;

	export var makeCaller: typeof _caller.makeCaller;
	export var makeMethodCaller: typeof _caller.makeMethodCaller;
}

publishNamespace('_nbind');

function _readAsciiString(ptr: number) {
	var endPtr = ptr;

	while(HEAPU8[endPtr++]);

	return(String.fromCharCode.apply('', HEAPU8.subarray(ptr, endPtr-1)));
}

@exportLibrary
class nbind {

	@dep('_nbind')
	static _nbind_register_pool(pageSize: number, usedPtr: number, pagePtr: number) {
		console.log(pageSize);
		console.log(usedPtr);
		console.log(pagePtr);
	}

	@dep('_nbind')
	static _nbind_register_method_getter_setter_id(methodID: number, getterID: number, setterID: number) {
		_nbind.MethodType.method = methodID;
		_nbind.MethodType.getter = getterID;
		_nbind.MethodType.setter = setterID;
	}

	@dep('_nbind')
	static _nbind_register_type(id: number, namePtr: number) {
		var name = _readAsciiString(namePtr);
		var constructorTbl: { [name: string]: { new(id: number, name: string): _type.BindType } } = {
			'bool': _nbind.BooleanType,
			'cbFunction &': _nbind.CallbackType,
			'std::string': _nbind.StringType,
			'_nbind_new': _nbind.CreateValueType
		};

		var constructor = constructorTbl[name] || _nbind.BindType;

		new constructor(id, name);
	}

	@dep('_nbind')
	static _nbind_register_types(dataPtr: number) {
		var count       = HEAPU32[dataPtr / 4];
		var idListPtr   = HEAPU32[dataPtr / 4 + 1] / 4;
		var sizeListPtr = HEAPU32[dataPtr / 4 + 2] / 4;
		var flagListPtr = HEAPU32[dataPtr / 4 + 3];

		var idList   = HEAPU32.subarray(idListPtr,   idListPtr   + count);
		var sizeList = HEAPU32.subarray(sizeListPtr, sizeListPtr + count);
		var flagList = HEAPU8. subarray(flagListPtr, flagListPtr + count);

		function createType(id: number, flag: number, size: number) {
			var isSignless = flag & 16;
			var isConst    = flag & 8;
			var isPointer  = flag & 4;
			var isFloat    = flag & 2;
			var isUnsigned = flag & 1;

			var name = isConst ? 'const ' : '';

			if(isSignless) name += 'char';
			else if(isPointer) {
				if(isUnsigned) name += 'un';
				name += 'signed char';
			} else {
				name += (
					(isUnsigned ? 'u' : '') +
					(isFloat ? 'float' : 'int') +
					(size * 8 + '_t')
				);
			}

			if(isPointer) {
				new _nbind.CStringType(id, name + ' *');
			} else {
				new _nbind.PrimitiveType(id, name, size, !!isUnsigned, !!isFloat);
			}
		}

		for(var num = 0; num < count; ++num) {
			createType(idList[num], flagList[num], sizeList[num]);
		}
	}

	@dep('_nbind', _readAsciiString, '__extends')
	static _nbind_register_class(idListPtr: number, namePtr: number) {
		var name = _readAsciiString(namePtr);
		var idList = HEAPU32.subarray(idListPtr / 4, idListPtr / 4 + 3);

		class Bound extends _nbind.Wrapper {
			constructor() {
				// super() never gets called here but TypeScript 1.8 requires it.
				if((false && super()) || !(this instanceof Bound)) {

					// Constructor called without new operator.
					// Make correct call with given arguments.
					// Few ways to do this work. This one should.
					// See http://stackoverflow.com/questions/1606797/use-of-apply-with-new-operator-is-this-possible

					return(new (Function.prototype.bind.apply(
						Bound, // arguments.callee
						Array.prototype.concat.apply([null], arguments)
					)));
				}

				super();

				_defineHidden(this.__nbindConstructor.apply(this, arguments))(this, '__nbindPtr');
			}

			@_defineHidden()
			__nbindConstructor: _nbind.Func;

			@_defineHidden()
			__nbindValueConstructor: _nbind.Func;
		}

		new _nbind.BindClass(idList[0], name, Bound);
		new _nbind.BindType(idList[1], name + ' *');
		new _nbind.BindType(idList[2], 'const ' + name + ' *');

		Module[name] = Bound;
	}

	@dep('_nbind')
	static _nbind_register_constructor(
		typeID: number,
		typeListPtr: number,
		typeCount: number,
		ptr: number,
		ptrValue: number
	) {
		var typeList = Array.prototype.slice.call(HEAPU32, typeListPtr / 4, typeListPtr / 4 + typeCount);

		var proto = (_nbind.typeList[typeID] as _class.BindClass).proto.prototype;

		_nbind.addMethod(
			proto,
			'__nbindConstructor',
			_nbind.makeCaller(null, 0, ptr, typeList),
			typeCount - 1
		);

		// First argument is a pointer to the C++ object to construct in place.
		// It fits in an unsigned int...

		typeList.splice(0, 1, 'void', 'uint32_t');

		_nbind.addMethod(
			proto,
			'__nbindValueConstructor',
			_nbind.makeCaller(null, 0, ptrValue, typeList),
			typeCount
		);
	}

	@dep('_nbind')
	static _nbind_register_destructor(typeID: number, ptr: number) {
		_nbind.addMethod(
			(_nbind.typeList[typeID] as _class.BindClass).proto.prototype,
			'free',
			_nbind.makeMethodCaller(ptr, 0, typeID, ['void']),
			0
		);
	}

	@dep('_nbind', _readAsciiString)
	static _nbind_register_function(
		typeID: number,
		typeListPtr: number,
		typeCount: number,
		ptr: number,
		namePtr: number,
		num: number,
		direct: number
	) {
		var name = _readAsciiString(namePtr);
		var typeList = Array.prototype.slice.call(HEAPU32, typeListPtr / 4, typeListPtr / 4 + typeCount);

		_nbind.addMethod(
			(_nbind.typeList[typeID] as _class.BindClass).proto as any,
			name,
			_nbind.makeCaller(ptr, num, direct, typeList),
			typeCount - 1
		);
	}

	@dep('_nbind', _readAsciiString)
	static _nbind_register_method(
		typeID: number,
		typeListPtr: number,
		typeCount: number,
		ptr: number,
		namePtr: number,
		num: number,
		methodType: number
	) {
		var name = _readAsciiString(namePtr);
		var typeList = Array.prototype.slice.call(HEAPU32, typeListPtr / 4, typeListPtr / 4 + typeCount);
		var proto = (_nbind.typeList[typeID] as _class.BindClass).proto.prototype;

		if(methodType == _nbind.MethodType.method) {
			_nbind.addMethod(
				proto,
				name,
				_nbind.makeMethodCaller(ptr, num, typeID, typeList),
				typeCount - 1
			);

			return;
		}

		// The C++ side gives the same name to getters and setters.
		var prefixMatcher = /^[Gg]et_?([A-Z]?)/;

		name = name.replace(prefixMatcher, (match: string, initial: string) => initial.toLowerCase());

		if(methodType == _nbind.MethodType.setter) {

			// A setter is always followed by a getter, so we can just
			// temporarily store an invoker in the property.
			// The getter definition then binds it properly.

			proto[name] = _nbind.makeMethodCaller(ptr, num, typeID, typeList);
		} else {
			Object.defineProperty(proto, name, {
				get: _nbind.makeMethodCaller(ptr, num, typeID, typeList),
				set: proto[name],
				enumerable: true,
				configurable: true
			});
		}
	}

	@dep('_nbind')
	static nbind_debug() {}

}
