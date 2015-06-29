var test = require('tap').test;
var testModule = require('bindings')({
	module_root: __dirname,
	bindings: 'test'
});

test('Methods and primitive types', function(t) {
	var Type = testModule.PrimitiveMethods;

	(function() {
		var obj = new Type();

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

		t.throws(function() {
			Type.strLengthStatic({});
		}, {message: 'Type mismatch'});

		// Constructing with or without "new" operator should work identically.
		obj = Type();

		gc();
		// Destructor should have incremented state.
		t.strictEqual(Type.getStateStatic(), 3);

		t.strictEqual(obj.negate(false), true);
	})();

	gc();
	// Destructor should have incremented state again.
	t.strictEqual(Type.getStateStatic(), 4);

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

	t.throws(function() {
		obj.t = 0;
	}, {message: 'Type mismatch'});

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

	t.type(Type.getCoord(), 'undefined');

	function Coord(x, y) {
		this.x = x;
		this.y = y;
	}

	Coord.prototype.fromJS = function(output) {
		console.error('FOOBAR ' + this.x + ' ' + this.y);
		output(this.x, this.y);
	}

	testModule.NBind.bind('Coord', Coord);

	var xy = Type.getCoord();

	t.strictEqual(xy.x, 1);
	t.strictEqual(xy.y, 2);

	xy.fromJS(function() {});
	Type.foo(xy);

	t.end();
});
