#!/usr/bin/env node

var path = require('path');
var nbind = require(path.resolve(__dirname, '..'));
var binding = nbind.init(path.resolve(process.cwd(), process.argv[2] || '.'));
var reflect = new (require(path.resolve(__dirname, '../dist/reflect.js')).Reflect)(binding);

process.stdout.write(require(path.resolve(__dirname, '../dist/todts.js')).dump(reflect));
