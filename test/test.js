var nbind = require('..');
var Int64 = require('../dist/int64.js').Int64;
var test = require('tap').test;

var binding = nbind.init();
var testModule = binding.lib;

var prepareGC;
var lost = null;

if(typeof(gc) == 'function') {
	prepareGC = function(obj) { gc(); }
} else {
	console.warn('Garbage collector is not accessible. Faking it...');
	console.warn('Run Node.js with --enable-gc to disable this warning.');
	console.warn('');

	prepareGC = function(obj) { lost = obj; }

	gc = function() {
		if(lost) lost.free();
		lost = null;
	}
}

test('Methods and primitive types', function(t) {
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

		t.strictEqual(Type.strLengthStatic(123), 3);

		obj = new Type(0, 'quux');
		t.strictEqual(Type.getStringStatic(), 'quux');
		t.strictEqual(obj.getString(), 'quux');
	})();

	t.end();
});

test('Constructors and destructors', function(t) {
	var Type = testModule.PrimitiveMethods;

	(function() {
		var obj = new Type();
		t.strictEqual(Type.getStateStatic(), 42);

		var obj = new Type(54);
		t.strictEqual(Type.getStateStatic(), 54);

		// Constructing with or without "new" operator should work identically.
		obj = Type();
		t.strictEqual(Type.getStateStatic(), 42);

		prepareGC(obj);

		obj = Type(54);
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

test('Functions', function(t) {
	t.strictEqual(testModule.incrementInt(1), 2);
	t.strictEqual(testModule.decrementInt(2), 1);

	t.end();
});

test('Getters and setters', function(t) {
	var Type = testModule.GetterSetter;
	var obj = new Type();

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

test('Callbacks', function(t) {
	var Type = testModule.Callback;

	t.type(Type.callVoidFunc(function() {}), 'undefined');
	t.strictEqual(Type.callNegate(function(x) {return(!x);}, false), true);
	t.strictEqual(Type.callIncrementInt(function(x) {return(x + 1);}, 1), 2);
	t.strictEqual(Type.callIncrementDouble(function(x) {return(x + 0.25);}, 0.5), 0.75);
	t.strictEqual(Type.callCatenate(function(x, y) {return(x + y);}, 'foo', 'bar'), 'foobar');

	t.throws(function() {
		Type.callNegate({}, true);
	}, {message: 'Type mismatch'});

	Type.callCStrings(function(foo, bar, baz) {
		t.strictDeepEqual([foo, bar, baz], ['foo', 'bar', 'baz']);
	});

	t.end();
});

test('Value objects', function(t) {
	var Type = testModule.Value;

//	t.throws(function() {
//		Type.getCoord()
//	}, {message: 'Value type JavaScript class is missing or not registered'});

	t.type(Type.getCoord(), 'object');

	function Coord(x, y) {
		this.x = x;
		this.y = y;
	}

	Coord.prototype.fromJS = function(output) {
		output(this.x, this.y);
	}

	binding.bind('Coord', Coord);

	var xy = Type.getCoord();

	t.strictEqual(xy.x, 60);
	t.strictEqual(xy.y, 25);

	xy.fromJS(function() {});
	Type.callWithCoord(function(a, b) {
		t.strictEqual(a.x, xy.x);
		t.strictEqual(a.y, xy.y);
		t.strictEqual(b.x, 123);
		t.strictEqual(b.y, 456);

		// TODO: if we don't return a Coord here as expected by the C++ side, it crashes!
		return(a);
	}, xy, new Coord(123, 456));

	t.end();
});

test('Pointers and references', function(t) {
	var Type = testModule.Reference;

	var own = new Type();
	var ptr = Type.getPtr();
	var ref = Type.getRef();
	var constPtr = Type.getConstPtr();
	var constRef = Type.getConstRef();

	var types = [ own, ptr, ref, constPtr, constRef ];

	for(var i = 0; i < types.length; ++i) {
		t.type(Type.readPtr(types[i]), 'undefined');
		t.type(Type.readRef(types[i]), 'undefined');

		if(types[i] == constPtr || types[i] == constRef) {
			t.throws(function() {
				Type.writePtr(types[i]);
			}, {message: 'Passing a const value as a non-const argument'});

			t.throws(function() {
				Type.writeRef(types[i]);
			}, {message: 'Passing a const value as a non-const argument'});
		} else {
			t.type(Type.writePtr(types[i]), 'undefined');
			t.type(Type.writeRef(types[i]), 'undefined');
		}
	}

	t.type(ptr.read(), 'undefined');
	t.type(ref.read(), 'undefined');

	t.type(ptr.write(), 'undefined');
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

test('Arrays', function(t) {
	var ArrayType = testModule.Array;
	var VectorType = testModule.Vector;

	var arr = [13, 21, 34];

	t.strictDeepEqual(ArrayType.getInts(), arr);
	t.strictDeepEqual(VectorType.getInts(), arr);

	t.strictDeepEqual(ArrayType.callWithInts(function(a) {
		t.strictDeepEqual(a, arr);
		return(arr);
	}, arr), arr);

	t.strictDeepEqual(VectorType.callWithInts(function(a) {
		t.strictDeepEqual(a, arr);
		return(arr);
	}, arr), arr);

	t.throws(function() {
		ArrayType.callWithInts(function(a) {}, [1, 2]);
	}, {message: 'Type mismatch'});

	var arr = ['foo', 'bar', 'baz'];

	t.strictDeepEqual(VectorType.callWithStrings(function(a) {
		t.strictDeepEqual(a, arr);
		return(arr);
	}, arr), arr);

	t.end();
});

test('Nullable', function(t) {
	var Type = testModule.Nullable;

	Type.foo(Type.getCoord());
	t.strictEqual(Type.getNull(), null);
	t.throws(function() {
		Type.foo(null);
	}, {message: 'Type mismatch'});

	Type.bar(null);

	t.end();
});

test('Strict conversion policy', function(t) {
	var typeList = [ testModule, testModule.StrictStatic, new testModule.Strict() ];

	for(var i = 0; i < typeList.length; ++i) {
		var Type = typeList[i];

		t.strictEqual(Type.testInt(1), 1);
		t.strictEqual(Type.testBool(true), true);
		t.strictEqual(Type.testString('foo'), 'foo');
		t.strictEqual(Type.testCString('foo'), 'foo');
		t.strictEqual(Type.testInt('123'), 123);
		t.strictEqual(Type.testBool(0), false);
		t.strictEqual(Type.testString(123), '123');
		t.strictEqual(Type.testCString(123), '123');

		t.strictEqual(Type.strictInt(1), 1);
		t.strictEqual(Type.strictBool(true), true);
		t.strictEqual(Type.strictString('foo'), 'foo');
		t.strictEqual(Type.strictCString('foo'), 'foo');

		t.throws(function() {
			Type.strictInt('123');
		}, {message: 'Type mismatch'});

		t.throws(function() {
			Type.strictBool(0);
		}, {message: 'Type mismatch'});

		t.throws(function() {
			Type.strictString(123);
		}, {message: 'Type mismatch'});

		t.throws(function() {
			Type.strictCString(123);
		}, {message: 'Type mismatch'});
	}

	t.end();
});

test('64-bit integers', function(t) {
	var Type = testModule.PrimitiveMethods;
	var lastDigit;

	var x = Type.ftoul(42);
	var y = Type.ftol(42);
	var z = Type.ftol(-42);

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

test('Overloaded functions', function(t) {
	var Type = testModule.Overload;
	var obj = new Type();

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

test('Buffers', function(t) {
	var Type = testModule.Buffer;
	var buf;

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
