var test = require('tap').test;

var nbind = require('..');

var testModule = nbind.module;

nbind.init(__dirname);

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

test('Getters and setters', function(t) {
	var Type = testModule.GetterSetter;
	var obj = new Type();

	t.strictEqual(obj.x, 1);
	t.strictEqual(obj.y, 2);
	t.strictEqual(obj.z, 3);
	t.strictEqual(obj.t, 'foobar');

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

	t.strictEqual(Type.callNegate(function(x) {return(!x);}, false), true);
	t.strictEqual(Type.callIncrementInt(function(x) {return(x + 1);}, 1), 2);

	t.throws(function() {
		Type.callNegate({}, true);
	}, {message: 'Type mismatch'});

	t.end();
});

test('Value objects', function(t) {
	var Type = testModule.Value;

	t.throws(function() {
		Type.getCoord()
	}, {message: 'Value type JavaScript class is missing or not registered'});

	function Coord(x, y) {
		this.x = x;
		this.y = y;
	}

	Coord.prototype.fromJS = function(output) {
		output(this.x, this.y);
	}

	nbind.bind('Coord', Coord);

	var xy = Type.getCoord();

	t.strictEqual(xy.x, 1);
	t.strictEqual(xy.y, 2);

	xy.fromJS(function() {});
	Type.callWithCoord(function(coord) {
		console.log(coord.x + ', ' + coord.y);
		// TODO: if we don't return Coord here as expected by the C++ side, it crashes!
		return(coord);
	}, xy);

	t.end();
});
