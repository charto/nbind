var nbind = require('..');
var test = require('tap').test;

var binding = nbind.init();
var testModule = binding.lib;

test('Argument checks', function(t) {
	t.throws(
		function() {
			testModule.PrimitiveMethods.incrementIntStatic(42, null);
		},
		new Error('Wrong number of arguments, expected 1')
	);

	t.end();
});
