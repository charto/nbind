declare var __dirname: any;
declare var require: any;
declare var process: any;
declare var gc: any;

declare var Buffer: any;

import * as nbind from '..';
import * as testLib from './testlib';
import {Int64} from '../dist/int64';
const test = require('tap').test;

const binding = nbind.init<typeof testLib>();
const testModule = binding.lib;

let prepareGC: (obj: any) => void;
var lost: any = null;

const global = (0 || eval)('this');

if(global.gc) {
	prepareGC = function(obj) { gc(); }
} else {
	console.warn('Garbage collector is not accessible. Faking it...');
	console.warn('Run Node.js with --expose-gc to disable this warning.');
	console.warn('');

	prepareGC = function(obj) { lost = obj; }

	global.gc = function() {
		if(lost) lost.free();
		lost = null;
	}
}

binding.toggleLightGC(true);

class Coord {
	constructor(x: number, y: number) {
		this.x = x;
		this.y = y;
	}

	fromJS(output: (x: number, y: number) => void) {
		output(this.x, this.y);
	}

	x: number;
	y: number;
}

type CoordJS = Coord;

declare module './testlib' {
	interface Coord extends CoordJS {}
}

test('Methods and primitive types', function(t: any) {
	var Type = testModule.PrimitiveMethods;

	(function() {
		var obj = new Type(0);

		t.strictEqual(Type.negateStatic(false), true);
		t.strictEqual(obj.negate(false), true);

		t.strictEqual(Type.incrementIntStatic(1), 2);
		t.strictEqual(obj.incrementInt(1), 2);

		t.type(Type.incrementStateStatic(), 'undefined');
		t.strictEqual(Type.getStateStatic(), 1);
		t.type(obj.incrementState(), 'undefined');
		t.strictEqual(obj.getState(), 2);

		t.strictEqual(Type.strLengthStatic('foo'), 3);
		t.strictEqual(obj.strLength('foobar'), 6);

		t.strictEqual(Type.catenateStatic('foo', 'bar'), 'foobar');
		t.strictEqual(obj.catenate('Java', 'Script'), 'JavaScript');
		t.strictEqual(obj.catenate2('Java', 'Script'), 'JavaScript');

		t.strictEqual(Type.strLengthStatic(123 as any as string), 3);

		obj = new Type(0, 'quux');
		t.strictEqual(Type.getStringStatic(), 'quux');
		t.strictEqual(obj.getString(), 'quux');
	})();

	t.end();
});

test('Constructors and destructors', function(t: any) {
	const Type = testModule.PrimitiveMethods;

	(function() {
		let obj = new Type();
		t.strictEqual(Type.getStateStatic(), 42);

		obj = new Type(54);
		t.strictEqual(Type.getStateStatic(), 54);

		// Constructing with or without "new" operator should work identically.
		obj = (Type as any as (p0?: number) => testLib.PrimitiveMethods)();
		t.strictEqual(Type.getStateStatic(), 42);

		prepareGC(obj);

		obj = (Type as any as (p0?: number) => testLib.PrimitiveMethods)(54);
		t.strictEqual(Type.getStateStatic(), 54);

		gc();

		// Destructor should have incremented state.
		t.strictEqual(Type.getStateStatic(), 55);

		prepareGC(obj);

		t.strictEqual(obj.negate(false), true);
	})();

	gc();

	// Destructor should have incremented state again.
	t.strictEqual(Type.getStateStatic(), 56);

	t.end();
});

test('Functions', function(t: any) {
	t.strictEqual(testModule.incrementInt(1), 2);
	t.strictEqual(testModule.decrementInt(2), 1);

	t.end();
});

test('Getters and setters', function(t: any) {
	const Type = testModule.GetterSetter;
	const obj = new Type();

	t.strictEqual(obj.x, 1);
	t.strictEqual(obj.y, 2);
	t.strictEqual(obj.z, 3);
	t.strictEqual(obj.t, 'foobar');
	t.strictEqual(obj.XYZ, 6);

	obj.y = 4;
	obj.z = 5;
	obj.t = 'foo';

	t.strictEqual(obj.y, 4);
	t.strictEqual(obj.z, 5);
	t.strictEqual(obj.t, 'foo');

//	TODO: Add a property taking an object and check that a wrong type throws.
//	t.throws(function() {
//		obj.t = 0;
//	}, {message: 'Type mismatch'});

	t.end();
});

test('Callbacks', function(t: any) {
	const Type = testModule.Callback;

	t.type(Type.callVoidFunc(function() {}), 'undefined');
	t.strictEqual(Type.callNegate(function(x: boolean) {return(!x);}, false), true);
	t.strictEqual(Type.callNegate2(function(x: boolean) {return(!x);}, false), true);
	t.strictEqual(Type.callAddInt(function(x: number, y: number) {return(x + y);}, 40, 2), 42);
	t.strictEqual(Type.callAddInt2(function(x: number, y: number) {return(x + y);}, 40, 2), 42);
	t.strictEqual(Type.callIncrementDouble(function(x: number) {return(x + 0.25);}, 0.5), 0.75);
	t.strictEqual(Type.callCatenate(function(x: string, y: string) {return(x + y);}, 'foo', 'bar'), 'foobar');
	t.strictEqual(Type.callCatenate2(function(x: string, y: string) {return(x + y);}, 'foo', 'bar'), 'foobar');

	t.throws(function() {
		Type.callNegate({} as any as (x: boolean) => boolean, true);
	}, {message: 'Type mismatch'});

	Type.callCStrings(function(foo: string, bar: string, baz: string) {
		t.strictDeepEqual([foo, bar, baz], ['foo', 'bar', 'baz']);
	});

	if(process.versions.modules > 14) {
		// Node 0.12 and earlier seem unable to catch exceptions from callbacks.

		t.throws(function() {
			Type.callNegate(function(x: boolean) { throw(new Error('Test error')); }, true);
		}, {message: 'Test error'});
	}

	t.end();
});

test('Value objects', function(t: any) {
	const Type = testModule.Value;

//	t.throws(function() {
//		Type.getCoord()
//	}, {message: 'Value type JavaScript class is missing or not registered'});

	t.type(Type.getCoord(), 'object');

	binding.bind('Coord', Coord);

	var xy = Type.getCoord();

	t.strictEqual(xy.x, 60);
	t.strictEqual(xy.y, 25);

	xy.fromJS(function() {});
	xy = Type.callWithCoord(function(a: Coord, b: Coord) {
		t.strictEqual(a.x, xy.x);
		t.strictEqual(a.y, xy.y);
		t.strictEqual(b.x, 123);
		t.strictEqual(b.y, 456);

		// TODO: if we don't return a Coord here as expected by the C++ side, it crashes!
		return(a);
	}, xy, new Coord(123, 456));

	t.strictEqual(xy.x, 60);
	t.strictEqual(xy.y, 25);

	t.end();
});

test('Pointers and references', function(t: any) {
	const Type = testModule.Reference;

	const own = new Type();
	const value = Type.getValue();
	const ptr = Type.getPtr();
	const ref = Type.getRef();
	const constPtr = Type.getConstPtr();
	const constRef = Type.getConstRef();

	const types = [ own, value, ptr, ref, constPtr, constRef ];

	for(var i = 0; i < types.length; ++i) {
		t.type(Type.readPtr(types[i]!), 'undefined');
		t.type(Type.readRef(types[i]!), 'undefined');

		if(types[i] == constPtr || types[i] == constRef) {
			t.throws(function() {
				Type.writePtr(types[i]!);
			}, {message: 'Passing a const value as a non-const argument'});

			t.throws(function() {
				Type.writeRef(types[i]!);
			}, {message: 'Passing a const value as a non-const argument'});
		} else {
			t.type(Type.writePtr(types[i]!), 'undefined');
			t.type(Type.writeRef(types[i]!), 'undefined');
		}
	}

	t.type(ptr!.read(), 'undefined');
	t.type(ref.read(), 'undefined');

	t.type(ptr!.write(), 'undefined');
	t.type(ref.write(), 'undefined');

	t.type(constPtr.read(), 'undefined');
	t.type(constRef.read(), 'undefined');

	t.throws(function() {
		constPtr.write();
	}, {message: 'Calling a non-const method on a const object'});

	t.throws(function() {
		constRef.write();
	}, {message: 'Calling a non-const method on a const object'});

	t.end();
});

test('Arrays', function(t: any) {
	const ArrayType = testModule.Array;
	const VectorType = testModule.Vector;

	const arr = [13, 21, 34];

	t.strictDeepEqual(ArrayType.getInts(), arr);
	t.strictDeepEqual(VectorType.getInts(), arr);

	t.strictDeepEqual(ArrayType.callWithInts(function(a: number[]) {
		t.strictDeepEqual(a, arr);
		return(arr);
	}, arr), arr);

	t.strictDeepEqual(VectorType.callWithInts(function(a: number[]) {
		t.strictDeepEqual(a, arr);
		return(arr);
	}, arr), arr);

	t.throws(function() {
		ArrayType.callWithInts(function(a: number[]) {}, [1, 2]);
	}, {message: 'Type mismatch'});

	const arr2 = ['foo', 'bar', 'baz'];

	t.strictDeepEqual(VectorType.callWithStrings(function(a: string[]) {
		t.strictDeepEqual(a, arr2);
		return(arr2);
	}, arr2), arr2);

	t.end();
});

test('Nullable', function(t: any) {
	const Type = testModule.Nullable;

	Type.foo(Type.getCoord()!);
	t.strictEqual(Type.getNull(), null);
	t.throws(function() {
		Type.foo(null as any as testLib.Coord);
	}, {message: 'Type mismatch'});

	Type.bar(null);

	t.end();
});

test('Strict conversion policy', function(t: any) {
	const typeList = [ testModule, testModule.StrictStatic, new testModule.Strict() ];

	for(let i = 0; i < typeList.length; ++i) {
		var Type = typeList[i];

		t.strictEqual(Type.testInt(1), 1);
		t.strictEqual(Type.testBool(true), true);
		t.strictEqual(Type.testString('foo'), 'foo');
		t.strictEqual(Type.testCString('foo'), 'foo');
		t.strictEqual(Type.testInt('123' as any as number), 123);
		t.strictEqual(Type.testBool(0 as any as boolean), false);
		t.strictEqual(Type.testString(123 as any as string), '123');
		t.strictEqual(Type.testCString(123 as any as string), '123');

		t.strictEqual(Type.strictInt(1), 1);
		t.strictEqual(Type.strictBool(true), true);
		t.strictEqual(Type.strictString('foo'), 'foo');
		t.strictEqual(Type.strictCString('foo'), 'foo');

		t.throws(function() {
			Type.strictInt('123' as any as number);
		}, {message: 'Type mismatch'});

		t.throws(function() {
			Type.strictBool(0 as any as boolean);
		}, {message: 'Type mismatch'});

		t.throws(function() {
			Type.strictString(123 as any as string);
		}, {message: 'Type mismatch'});

		t.throws(function() {
			Type.strictCString(123 as any as string);
		}, {message: 'Type mismatch'});
	}

	t.end();
});

test('Inheritance', function(t: any) {
	const A = testModule.InheritanceA;
	const B = testModule.InheritanceB;
	const C = testModule.InheritanceC;
	const D = testModule.InheritanceD;

	const d: any = new D();

	t.ok(d instanceof A);
	t.ok(d instanceof B || d instanceof C);
	t.ok(d instanceof D);

	t.ok(d.a instanceof A);
	t.ok(d.b instanceof A);
	t.ok(d.c instanceof A);

	t.ok(d.b instanceof B);
	t.ok(d.c instanceof C);

	t.ok(d.b.a instanceof A);
	t.ok(d.c.a instanceof A);

	t.throws(function() {
		d.useA.call(new Date());
	}, {message: 'Type mismatch'});

	t.strictEqual(d.useA(), 1);
	t.strictEqual(d.useB(), 2);
	t.strictEqual(d.useC(), 3);
	t.strictEqual(d.useD(), 4);

	t.strictEqual(d.a.useA(), 1);
	t.strictEqual(d.b.useB(), 2);
	t.strictEqual(d.c.useC(), 3);

	t.strictEqual(d.b.a.useA(), 1);
	t.strictEqual(d.c.a.useA(), 1);

	t.strictEqual(A.staticA(d), 1);
	t.strictEqual(A.staticA(d.b), 1);
	t.strictEqual(A.staticA(d.c), 1);

	t.strictEqual(B.staticB(d), 2);
	t.strictEqual(B.staticB(d.b), 2);
	t.throws(function() {
		B.staticB(d.c as any);
	}, {message: 'Type mismatch'});

	t.strictEqual(C.staticC(d), 3);
	t.strictEqual(C.staticC(d.c), 3);
	t.throws(function() {
		C.staticC(d.b as any);
	}, {message: 'Type mismatch'});

	t.end();
});

test('64-bit integers', function(t: any) {
	const Type = testModule.PrimitiveMethods;
	let lastDigit: string;

	let x = Type.ftoul(42);
	let y = Type.ftol(42);
	let z = Type.ftol(-42);

	t.strictEqual(Type.ultof(x), 42);
	t.strictEqual(Type.ltof(y), 42);
	t.strictEqual(Type.ltof(z), -42);

	for(var j = 0; j < 2; ++j) {
		for(var n = 2, i = 1; i < 63; ++i) {
			x = Type.ftoull(n);
			y = Type.ftoll(n);
			z = Type.ftoll(-n);

			t.strictEqual(Type.ulltof(x), n);
			t.strictEqual(Type.lltof(y), n);
			t.strictEqual(Type.lltof(z), -n);

			if(j) {
				lastDigit = '5137'.charAt(i & 3);
				t.strictEqual(('' + x).substr(-1), lastDigit);
				t.strictEqual(('' + y).substr(-1), lastDigit);
				t.strictEqual(('' + z).substr(-1), lastDigit);
			}

			n *= 2;
		}

		binding.bind('Int64', Int64);
	}

	t.end();
});

test('Overloaded functions', function(t: any) {
	const Type = testModule.Overload;
	const obj = new Type();

	t.strictEqual(obj.test(0), 1);
	t.strictEqual(obj.test2(0, 0), 2);

	t.strictEqual(obj.testConst(0), 1);
	t.strictEqual(obj.testConst2(0, 0), 2);

	t.strictEqual(Type.testStatic(0), 1);
	t.strictEqual(Type.testStatic2(0, 0), 2);

	t.strictEqual(testModule.multiTest(0), 1);
	t.strictEqual(testModule.multiTest2(0, 0), 2);

	t.end();
});

test('Smart pointers', function(t: any) {
	const Type = testModule.Smart;

	const obj = Type.make(31337);

	obj!.test();
	Type.testStatic(obj!);
	Type.testShared(obj!);

	obj!.free!();
	// obj.free();

	t.end();
});

test('Buffers', function(t: any) {
	const Type = testModule.Buffer;
	let buf: any;

	if(ArrayBuffer && (typeof(process) != 'object' || typeof(process.versions) != 'object' || process.versions.modules >= 14)) {
		buf = new ArrayBuffer(16);
		var view = new Uint8Array(buf);

		for(var i = 0; i < 16; ++i) view[i] = i;

		t.strictEqual(Type.sum(buf), 120);
		t.strictEqual(Type.sum(view), 120);
		t.strictEqual(Type.sum(view.subarray(2, 12)), 65);
		t.strictEqual(Type.sum(new Uint8Array(buf, 2, 12)), 90);

		Type.mul2(buf);

		t.strictEqual(Type.sum(buf), 240);

		Type.mul2(view);

		t.strictEqual(Type.sum(view), 480);
	}

	if(Buffer) {
		buf = Buffer.alloc ? Buffer.alloc(16) : new Buffer(16);

		for(var i = 0; i < 16; ++i) buf[i] = i;

		t.strictEqual(Type.sum(buf), 120);

		Type.mul2(buf);

		t.strictEqual(Type.sum(buf), 240);
	}

	t.end();
});

test('Reflection', function(t: any) {
	const fs = require('fs');
	const path = require('path').resolve(__dirname, 'reflect.txt');
	const reflect = new (require('../dist/reflect.js').Reflect)(binding);

	t.strictEqual(
		reflect.dumpPseudo().replace(/int64/g, 'int32'),
		fs.readFileSync(path, 'utf-8').replace(/int64/g, 'int32')
	);

	t.end();
});
