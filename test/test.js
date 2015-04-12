var test = require('tap').test;
var testModule = require('bindings')({
	module_root: __dirname,
	bindings: 'test'
});

var Prim = testModule.PrimitiveMethods;
var prim = new Prim();

test('Methods and primitive types', function(t) {
	t.strictEqual(Prim.negateStatic(false), true);
	t.strictEqual(prim.negate(false), true);

	t.strictEqual(Prim.incrementIntStatic(1), 2);
	t.strictEqual(prim.incrementInt(1), 2);

	t.type(Prim.incrementStateStatic(), 'undefined');
	t.strictEqual(Prim.getStateStatic(), 1);
	t.type(prim.incrementState(), 'undefined');
	t.strictEqual(prim.getState(), 2);

	t.end();
});
