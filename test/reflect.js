var nbind = require('..');
var binding = nbind.init('test/v8');
var reflect = new (require('../dist/reflect.js').Reflect)(binding);

console.log(reflect.dumpPseudo());
