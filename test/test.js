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
	})();

	gc();
	// Destructor should have incremented state again.
	t.strictEqual(Type.getStateStatic(), 3);

	t.end();
});

test('Getters and setters', function(t) {
	var Type = testModule.GetterSetter;
	var obj = new Type();

	t.strictEqual(obj.x, 1);
	t.strictEqual(obj.y, 2);
	t.strictEqual(obj.z, 3);

	obj.y = 4;
	obj.z = 5;

	t.strictEqual(obj.y, 4);
	t.strictEqual(obj.z, 5);

	t.end();
});

test('Callbacks', function(t) {
	var Type = testModule.Callback;

	t.strictEqual(Type.callNegate(function(x) {return(!x);}, false), true);
	t.strictEqual(Type.callIncrementInt(function(x) {return(x + 1);}, 1), 2);

	t.end();
});
