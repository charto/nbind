#!/usr/bin/env node

var path = require('path');
var nbind = require(path.resolve(__dirname, '..'));

var shim = true;
var target = process.argv[2];

if(target == '--no-shim') {
	shim = false;
	target = process.argv[3];
} else if(process.argv[3] == '--no-shim') {
	shim = false;
}

var binding = nbind.init(path.resolve(process.cwd(), target || '.'));
var reflect = new (require(path.resolve(__dirname, '../dist/reflect.js')).Reflect)(binding);

process.stdout.write(require(path.resolve(__dirname, '../dist/todts.js')).dump({
	reflect: reflect,
	shim: shim
}));
