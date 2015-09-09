var test = require('tap').test;
var testModule;
var nbind;

eval(require('fs').readFileSync('foo.js', 'utf-8'));
Module.ccall('nbind_init');
testModule = Module;

test('Methods and primitive types', function(t) {
	var Type = testModule.PrimitiveMethods;

	(function() {
		var obj = new Type(Module._malloc(100));

		t.strictEqual(!!Type.negateStatic(false), true);
		t.strictEqual(!!obj.negate(false), true);

		t.strictEqual(Type.incrementIntStatic(1), 2);
		t.strictEqual(obj.incrementInt(1), 2);

		t.type(Type.incrementStateStatic(), 'undefined');
		t.strictEqual(Type.getStateStatic(), 1);
		t.type(obj.incrementState(), 'undefined');
		t.strictEqual(obj.getState(), 2);
	})();

	t.end();
});
