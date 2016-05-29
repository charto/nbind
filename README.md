[![nbind flowchart](doc/images/diagram.png)](doc/images/diagram.png)

`nbind` is a set of headers that make your C++11 library accessible from JavaScript.
With a single `#include` statement, your C++ compiler generates the necessary bindings
without any additional tools. Your library is then usable as a Node.js addon or,
if compiled to asm.js with [Emscripten](http://emscripten.org),
directly in web pages without any plugins.

`nbind` works with the [autogypi](https://github.com/charto/autogypi) build tool,
which sets up `node-gyp` to compile your library without needing any configuration
(other than listing your source code file names).

`nbind` is based on templates and macros inspired by
[embind](http://kripken.github.io/emscripten-site/docs/porting/connecting_cpp_and_javascript/embind.html).

C++ everywhere in 5 easy steps using Node.js, `nbind` and [autogypi](https://github.com/charto/autogypi):

<table>
<tr>
	<th>Starting point</th>
	<th>Step 1 - bind</th>
	<th>Step 2 - prepare</th>
</tr><tr>
<td valign="top">Original C++ code <code>hello.cc</code>:<br>
<pre>#include &lt;string&gt;
#include &lt;iostream&gt;
&nbsp;
struct Greeter {
  static void sayHello(
    std::string name
  ) {
    std::cout
      &lt;&lt; "Hello, "
      &lt;&lt; name &lt;&lt; "!\n";
  }
};</pre></td>
<td valign="top">List your classes and methods:<br>
<pre>// Your original code here
&nbsp;
// Add these below it:
&nbsp;
#include "nbind/nbind.h"
&nbsp;
NBIND_CLASS(Greeter) {
    method(sayHello);
}</pre></td>
<td valign="top">Add scripts to <code>package.json</code>:<br>
<pre>{
  "scripts": {
    "emcc-path": "emcc-path",
    "autogypi": "autogypi",
    "node-gyp": "node-gyp"
  }
}</pre></td>
</tr><tr>
	<th>Step 3 - install</th>
	<th>Step 4 - build</th>
	<th>Step 5 - use!</th>
</tr><tr>
<td valign="top">Run on the command line:<br>
<pre>npm install --save \
  nbind autogypi node-gyp
&nbsp;
npm run -- autogypi \
  --init-gyp \
  -p nbind -s hello.cc</pre></td>
<td valign="top">Compile to native binary:<br>
<pre>npm run node-gyp \
  configure build</pre>
Or to Asm.js:<br>
<pre>npm run node-gyp \
  configure build \
  --asmjs=1</pre></td>
<td valign="top">Call from Node.js:<br>
<pre>var nbind = require('nbind');
var lib = nbind.init();
&nbsp;
lib.Greeter.sayHello('you');</pre>
</td></tr>
</table>

The above is **all** of the required code. Just copy and paste in the mentioned files and prompts or take a shortcut:

```bash
git clone https://github.com/charto/nbind-example-minimal.git
cd nbind-example-minimal
npm install && npm test
```

See it run! You need Node.js 0.10.x - 6.x.x (newer may also work), Python 2.7 (required by `node-gyp`)
and one of the following C++ compilers:

- GCC 4.8 or above
- Clang 3.6 or above
- Emscripten 1.35.0 or above
- Visual Studio 2015 ([The Community version](https://www.visualstudio.com/en-us/products/visual-studio-community-vs.aspx) is fine)

Features
--------

`nbind` supports:

- Linux, OS X, Windows.
- Node.js, asm.js, Electron.

It allows you to:

- Bind unlimited C++ classes.
- Bind C++ methods simply by mentioning their names.
- Auto-detect argument and return types from C++ declarations.
- [Automatically convert types](#type-conversion) between languages.
- Call bound C++ methods from JavaScript with type checking.
- Pass JavaScript callbacks to C++ and call them with any types.
- Pass instances of compatible classes by value between languages (through the C++ stack).

Works on your platform
----------------------

<table>
	<tr>
		<th>Target</th>
		<th colspan=2>Development platform</th>
	</tr><tr>
		<th></th>
		<th>Linux / OS X</th>
		<th>Windows</th>
	</tr><tr>
		<td>Native</td>
		<td>
			<a href="http://travis-ci.org/charto/nbind">
				<img src="https://travis-ci.org/charto/nbind.svg?branch=master" alt="Build status">
			</a>
		</td>
		<td>
			<a href="https://ci.appveyor.com/project/jjrv/nbind/branch/master">
				<img src="https://ci.appveyor.com/api/projects/status/xu5ooh1m3mircpde/branch/master?svg=true" alt="Build status">
			</a>
		</td>
	</tr><tr>
		<td>Asm.js</td>
		<td>
			<a href="http://travis-ci.org/charto/nbind-ci-emscripten">
				<img src="https://travis-ci.org/charto/nbind-ci-emscripten.svg?branch=master" alt="Build status">
			</a>
		</td>
		<td>Tested manually</td>
	</tr>
</table>

[![dependency status](https://david-dm.org/charto/nbind.svg)](https://david-dm.org/charto/nbind)
[![npm version](https://img.shields.io/npm/v/nbind.svg)](https://www.npmjs.com/package/nbind)

Roadmap
-------

More is coming! Work is ongoing to:

- Automatically generate TypeScript `.d.ts` definition files from C++ code for IDE autocompletion and compile-time checks of JavaScript side code.
- Support native Android and iPhone apps.

Warning to git committers: rebase is used within feature branches (but not master).

Type conversion
---------------

Parameters and return values of function calls between languages
are automatically converted between equivalent types:

| JavaScript | C++                               |
| ---------- | --------------------------------- |
| number     | (un)signed char, short, int, long |
| number     | float, double                     |
| boolean    | bool                              |
| string     | (const) (unsigned) char *         |
| string     | std::string                       |
| Array      | std::vector&lt;type&gt;           |
| Array      | std::array&lt;type, size&gt;      |
| Function   | nbind::cbFunction<br>(only as a parameter) |
| Instance of any prototype<br>(with a fromJS method) | Instance of any class<br>(with a toJS method) |

What?
-----

To create bindings from a class X with a constructor taking 2 ints and a method Y with pretty much any kind of arguments and optional return value, you just need:

```
NBIND_CLASS(X) {
    construct<int, int>();

    method(Y);
}
```

Example
-------

Let's look at a concrete example. Suppose we have a C++ class:

```C++
// test.cc part 1
// Get definition of nbind::cbFunction
#include "nbind/api.h"

class Test {

public:

    // Can be constructed with or without an initial state.
    Test(int newState = 42) : state(newState) {}

    // Getter and setter for private member.
    int getState() { return(state); }
    void setState(int newState) { state = newState; }

    // Call a callback function passing it the state variable and return the result.
    int callWithState(nbind::cbFunction &callback) {
        // The template argument defines what type to cast the return value to.
        return(callback.call<int>(state));
    }

    // Test returning a string from a static function.
    static const char *toString() { return("Test class"); }

private:

    int state;
};
```

Creating bindings for it is ridiculously easy, by adding the following in a source (not header) file:

```C++
// test.cc part 2
#include "nbind/BindingShort.h"

// Avoid errors if we're not compiling a Node.js module.
#ifdef NBIND_CLASS

NBIND_CLASS(Test) {
    // Both possible constructors, nbind will choose one based on how it's called.
    construct<>();
    construct<int>();

    // Getter and setter pair, nbind figures out it's a variable called "state".
    getset(getState, setState);

    // The methods, one taking a callback and the other static.
    method(callWithState);
    method(toString);
}

#endif
```

Then we can call it from JavaScript:

```javascript
// test.js
var nbind = require('nbind');
var pkg = nbind.module;

nbind.init(__dirname);

// Prints "Test class"
console.log('' + pkg.Test);

var obj = new pkg.Test();

// Prints 42
console.log(obj.state);

// Prints 43
console.log(++obj.state);

// Prints 86
console.log(obj.callWithState(function(state) {
	return(state * 2);
}));
```

Only a bit more configuration is needed to get everything installed, compiled and running.

`package.json` to list what needs to be installed and run:
```json
{
  "name": "nbind-demo",
  "scripts": {
    "install": "autogypi && node-gyp configure && node-gyp build",
    "test": "node test.js"
  },
  "dependencies": {
    "autogypi": "^0.1.0",
    "nbind": "^0.1.0"
  }
}
```

`autogypi.json` to list dependencies for autogypi and configure them automatically for node-gyp:
```json
{
	"dependencies": [
		"nbind"
	],
	"output": "auto.gypi"
}
```

`binding.gyp` to tell `node-gyp` what to do:
```json
{
	"targets": [
		{
			"includes": ["auto.gypi"],
			"sources": [
				"test.cc"
			]
		}
	]
}
```

That's it! Install, compile, test:
```bash
npm install
npm test
```

If it doesn't work, your `npm` might be too old. Try compiling again after running this command:

```bash
npm install -g npm
```

Usage and syntax
================

JavaScript side
---------------

`nbind` is intended to be used together with [autogypi](https://www.npmjs.com/package/autogypi) for easier dependency management. Declare it as a dependency in `autogypi.json`, run `autogypi` and include the resulting `auto.gypi` from your `binding.gyp` file. See the example section above for sample contents of these configuration files.

Your code might depend on other npm packages that also contain `nbind` exports. `autogypi` will make sure that when you run `node-gyp`, their sources will also get compiled and their includes will be in the include path when your code is compiled. When you run `node-gyp` it produces a module with all the exported classes from all packages you've included. Then you just do in the root directory of your package:

```javascript
var nbind = require('nbind');
nbind.init(__dirname);
```

This allows nbind to find the module you compiled. Afterwards, `nbind.module` will contain all the exported classes.

C++ nbind headers
-----------------

Use `#include "nbind/BindingShort.h"` at the end of your source file with only the bindings after it. The header defines macros with names like `construct` that are otherwise likely to break the code implementing your C++ class.

It's OK to include the file also when not targeting any JavaScript environment. `node-gyp` defines `BUILDING_NODE_EXTENSION` and Emscripten defines `EMSCRIPTEN` so when those are undefined, the include file does nothing.

Use `#include "nbind/api.h"` in your header files to use types in the `nbind` namespace if you need to report errors without throwing exceptions, or want to pass around callbacks or value objects.

You can use the `NBIND_ERR("message here");` macro to report an error before returning. It will be thrown as an error on the JavaScript side (C++ environments like Emscripten may not support throwing exceptions, but the JavaScript side code will).

Classes and constructors
------------------------

You can use an `#ifdef NBIND_CLASS` guard to skip your `nbind` export definitions when the includes weren't loaded.

The `NBIND_CLASS(className)` macro takes the name of your C++ class as an argument (without any quotation marks), and exports it to JavaScript using the same name. It's followed by a curly brace enclosed block of method exports, as if it was a function definition.

Constructors are exported with a macro call `construct<types...>();` where `types` is a comma-separated list of arguments to the constructor, such as `int, int`. Calling `construct` multiple times allows overloading it, but **each overload must have a different number of arguments**.

Methods and properties
----------------------

Methods are exported with a macro call `method(methodName);` which takes the name of the method as an argument (without any quotation marks). It gets exported to JavaScript with the same name. If the method is `static`, it becomes a property of the JavaScript constructor function and can be accessed like `className.methodName()`. Otherwise it becomes a property of the prototype and can be accessed like `obj = new className(); obj.methodName();`

Property getters are exported with a macro call `getter(getterName)` which takes the name of the getter method. `nbind` automatically strips a `get`/`Get`/`get_`/`Get_` prefix and converts the next letter to lowercase, so for example `getX` and `get_x` both would become getters of `x` to be accessed like `obj.x`

Property setters are exported together with getters using a macro call `getset(getterName, setterName)` which works much like `getter(getterName)` above. Both `getterName` and `setterName` are mangled individually so you can pair `getX` with `set_x` if you like. From JavaScript, `++obj.x` would then call both of them.

Callbacks and value objects
---------------------------

Callbacks can be passed to C++ methods by simply adding an argument of type `nbind::cbFunction &` to their declaration. They can be called with any number of any supported types without having to declare in any way what they accept. The JavaScript code will receive the parameters as JavaScript variables to do with them as it pleases. A callback argument `arg` can be called like `arg("foobar", 42);` in which case the return value is ignored. If the return value is needed, it must be called like `arg.call<type>("foobar", 42);` where `type` is the desired C++ type that the return value should be converted to.

Value objects are the most advanced concept supported so far. They're based on a `toJS` function on the C++ side and a `fromJS` function on the JavaScript side. Both receive a callback as an argument, and calling it with any parameters calls the constructor of the equivalent type in the other language (the bindings use [SFINAE](https://en.wikipedia.org/wiki/Substitution_failure_is_not_an_error) to detect the toJS function and turn the class into a value type). The callback on the C++ side is of type `nbind::cbOutput`. Value objects are passed **by value** on the C++ side to and from the exported function. `nbind` uses C++11 move semantics to avoid creating some additional copies on the way.

The equivalent JavaScript constructor must be registered on the JavaScript side by calling `nbind.bind('CppClassName', JSClassName);`, so `nbind` knows which types to translate between each other.

So if we have a file `test.cc` (can be tested by replacing the file with the same name in the example above):

```C++
#include "nbind/api.h"

int copies = 0;
int moves = 0;

// Coordinate pair, a value type we'll bind to an equivalent JavaScript type.

class Coord {

public:

    Coord(int x = 0, int y = 0) : x(x), y(y) {}

    // Let's count copy and move constructor usage.

    Coord(Coord &&other) : x(other.x), y(other.y) { ++copies; }

    Coord(const Coord &other) : x(other.x), y(other.y) { ++moves; }

    void addFrom(const Coord &other) {
        x += other.x;
        y += other.y;
    }

    // This is the magic function for nbind, which calls a constructor in JavaScript.

    void toJS(nbind::cbOutput output) {
        output(x, y);
    }

private:

    int x, y;
};

// This just sums coordinate pairs and returns them for testing.

class Accumulator {

public:

    Coord add(Coord other) {
        xy.addFrom(other);

        // a copy is made here.
        return(xy);
    }

    // This is a convenient place to read debug output, since we can't call methods of Coord
    // from JavaScript (it sees methods defined on the JavaScript object instead).

    int getCopies() { return(copies); }

    int getMoves() { return(moves); }

private:

    Coord xy;
};

#include "nbind/BindingShort.h"

#ifdef NBIND_CLASS

NBIND_CLASS(Coord) {
    construct<>();
    construct<int, int>();
}

NBIND_CLASS(Accumulator) {
    construct<>();

    method(add);
    getter(getCopies);
    getter(getMoves);
}

#endif
```

We can use it from a JavaScript file `test.js` like so:

```javascript
var nbind = require('nbind');

nbind.init(__dirname);

function Coord(x, y) {
    this.x = x;
    this.y = y;
}

Coord.prototype.fromJS = function(output) {
    output(this.x, this.y);
}

nbind.bind('Coord', Coord);

var accu = new nbind.module.Accumulator();

// Prints: { x: 2, y: 4 }
console.log(accu.add(new Coord(2, 4)));

// Prints: { x: 12, y: 34 }
console.log(accu.add(new Coord(10, 30)));

// Prints: 4 copies, 2 moves.
console.log(accu.copies + ' copies, ' + accu.moves + ' moves.');
```

License
=======

[The MIT License](https://raw.githubusercontent.com/charto/nbind/master/LICENSE)

Copyright (c) 2014-2016 BusFaster Ltd
