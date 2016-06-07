[Quick start](#quick-start) |
[Requirements](#requirements) |
[Features](#features) |
[User guide](#user-guide) |
[Contributing](#contributing) |
[License](#license)

[![nbind flowchart](doc/images/diagram.png)](doc/images/diagram.png)

`nbind` is a set of headers that make your C++11 library accessible from JavaScript.
With a single `#include` statement, your C++ compiler generates the necessary bindings
without any additional tools. Your library is then usable as a Node.js addon or,
if compiled to asm.js with [Emscripten](http://emscripten.org),
directly in web pages without any plugins.

`nbind` works with the [autogypi](https://github.com/charto/autogypi) dependency management tool,
which sets up `node-gyp` to compile your library without needing any configuration
(other than listing your source code file names).

`nbind` is **MIT licensed** and based on templates and macros inspired by
[embind](http://kripken.github.io/emscripten-site/docs/porting/connecting_cpp_and_javascript/embind.html).

Quick start
===========

C++ everywhere in 5 easy steps using Node.js, `nbind` and [autogypi](https://github.com/charto/autogypi):

<table>
<tr>
	<th>Starting point</th>
	<th>Step 1 - bind</th>
	<th>Step 2 - prepare</th>
</tr><tr>
<td valign="top">Original C++ code <a href="https://raw.githubusercontent.com/charto/nbind-example-minimal/master/hello.cc"><code>hello.cc</code></a>:<br>
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
<td valign="top">List your <a href="#classes-and-constructors">classes</a> and <a href="#methods-and-properties">methods</a>:<br>
<pre>// Your original code here
&nbsp;
// Add these below it:
&nbsp;
#include "nbind/nbind.h"
&nbsp;
NBIND_CLASS(Greeter) {
  method(sayHello);
}</pre></td>
<td valign="top"><a href="#creating-your-project">Add scripts</a> to <a href="https://raw.githubusercontent.com/charto/nbind-example-minimal/master/package.json"><code>package.json</code></a>:<br>
<pre>{
  "scripts": {
    "autogypi": "autogypi",
    "node-gyp": "node-gyp",
    "emcc-path": "emcc-path",
    "copyasm": "copyasm"
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
<pre>npm run -- node-gyp \
  configure build</pre>
Or to Asm.js:<br>
<pre>npm run -- node-gyp \
  configure build \
  --asmjs=1</pre></td>
<td valign="top">Call from Node.js:<br>
<pre>var nbind = require('nbind');
var lib = nbind.init().lib;
&nbsp;
lib.Greeter.sayHello('you');</pre>
Or from a web browser (<a href="#using-in-web-browsers">see below</a>).
</td></tr>
</table>

The above is **all** of the required code. Just copy and paste in the mentioned files and prompts or take a shortcut:

```bash
git clone https://github.com/charto/nbind-example-minimal.git
cd nbind-example-minimal
npm install && npm test
```

See it run!

(Note: [nbind-example-universal](https://github.com/charto/nbind-example-universal)
is a better starting point for development)

Requirements
============

You need:

- [Node.js](https://nodejs.org/en/) 0.10.x - 6.x.x (newer may also work).
- Python 2.7, NOT 3.x (required by `node-gyp`, [see instructions](https://github.com/nodejs/node-gyp#installation)).

And one of the following C++ compilers:

- GCC 4.8 or above.
- Clang 3.6 or above.
- Emscripten 1.35.0 or above.
- Visual Studio 2015 ([the Community version](https://www.visualstudio.com/en-us/products/visual-studio-community-vs.aspx) is fine).

Features
========

`nbind` allows you to:

- Use your C++ API from JavaScript without any extra effort.
  - From **Node.js**, **Electron** and web browsers (using asm.js on **Chrome**, **Firefox** and **Edge**).
  - On Linux, OS X and Windows.
  - Without changes to your C++ code. Simply add a separate short description at the end.
- Distribute both **native** code and an **asm.js** fallback binary.

In more detail:

- Export multiple C++ classes, even ones not visible from other files.
- Export C++ methods simply by mentioning their names.
- Auto-detect argument and return types from C++ declarations.
- [Automatically convert types](#type-conversion) and data structures between languages.
- Call C++ methods from JavaScript with type checking.
- Pass JavaScript callbacks to C++ and call them with any types.
- Pass instances of compatible classes by value between languages (through the C++ stack).

The goal is to provide a **stable API** for binding C++ to JavaScript.
All internals related to JavaScript engines are hidden away,
and a single API already supports *extremely* different platforms.

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

Future `0.x.y` versions *should* remain completely backwards-compatible between matching `x` and otherwise with minor changes.
Breaking changes will be listed in release notes of versions where `y` equals `0`.

Contributing
============

Pull requests are very welcome.
When developing new features, writing tests first works best.
Please report issues through Github.

Warning: rebase is used within develop and feature branches (but not master).

User guide
==========

- [Installing the examples](#installing-the-examples)
- [Creating your project](#creating-your-project)
- [Configuration](#configuration)
- [Calling from Node.js](#calling-from-nodejs)
- [Using nbind headers](#using-nbind-headers)
- [Classes and constructors](#classes-and-constructors)
- [Methods and properties](#methods-and-properties)
- [Getters and setters](#getters-and-setters)
- [Passing data structures](#passing-data-structures)
- [Callbacks](#callbacks)
- [Using objects](#using-objects)
- [Type conversion](#type-conversion)
- [Error handling](#error-handling)
- [Publishing on npm](#publishing-on-npm)
- [Shipping an asm.js fallback](#shipping-an-asmjs-fallback)
- [Using in web browsers](#using-in-web-browsers)
- [Using with TypeScript](#using-with-typescript)
- [Debugging](#debugging)

Installing the examples
-----------------------

`nbind` examples shown in this user guide are also available to download
for easier testing as follows:

Extract [this zip package](https://github.com/charto/nbind-examples/archive/master.zip) or run:

```bash
git clone https://github.com/charto/nbind-examples.git
```

Enter the examples directory and install:

```bash
cd nbind-examples
npm install
```

Creating your project
---------------------

Once you have all [requirements](#requirements) installed, run:

```bash
npm init
npm install --save nbind autogypi node-gyp
```

`nbind`, `autogypi` and `node-gyp` are all needed to compile
a native Node.js addon from source when installing it.
If you only distribute an asm.js version, you can use
`--save-dev` instead of `--save` because users won't need to compile it.

Next, to run commands without installing them globally, it's practical
to add them in the `scripts` section of your `package.json` that `npm init`
just generated. Let's add an install script as well:

```json
  "scripts": {
    "autogypi": "autogypi",
    "node-gyp": "node-gyp",
    "emcc-path": "emcc-path",
    "copyasm": "copyasm",

    "install": "autogypi && node-gyp configure build"
  }
```

`emcc-path` is needed internally by `nbind` when compiling for asm.js.
It fixes some command line options that `node-gypi` generates on OS X
and the Emscripten compiler doesn't like.
You can leave it out if only compiling native addons.

The `install` script runs when anyone installs your package.
It calls `autogypi` and then uses `node-gyp` to compile a native addon.

`autogypi` uses npm package information to set correct include paths
for C/C++ compilers. It's needed when distributing addons on npm
so the compiler can find header files from the `nbind` and `nan` packages
installed on the user's machine. Initialize it like this:

```bash
npm run -- autogypi --init-gyp -p nbind -s hello.cc
```

Replace `hello.cc` with the name of your C++ source file.
You can add multiple `-s` options, one for each source file.

The `-p nbind` means the C++ code uses `nbind`. Multiple `-p`
options can be added to add any other packages compatible with `autogypi`.

The `--init-gyp` command generates files `binding.gyp` and `autogypi.json`
that you should distribute with your package, so that `autogypi` and `node-gyp`
will know what to do when the `install` script runs.

Now you're ready to start writing code and compiling.

Configuration
-------------

Refer to [autogypi documentation](https://github.com/charto/autogypi#readme)
to set up dependencies of your package, and how other packages
should include it if it's a library usable directly from C++.

`--asmjs=1` is the only existing configuration option for `nbind` itself.
You pass it to `node-gyp` by calling it like `node-gyp configure build --asmjs=1`.
It compiles your package using Emscripten instead of your default C++ compiler
and produces asm.js output.

Calling from Node.js
--------------------

First `nbind` needs to be initialized by calling `nbind.init` which takes
the following optional arguments:

- Base path under which to look for compiled binaries.
  Default is `process.cwd()` and `__dirname` is a good alternative.
- Binary code exports object. Any classes from C++ API exported using `nbind`
  will be added as members. Default is an empty object.
  Any existing options will be seen by asm.js code and can be used to
  [configure Emscripten output](https://kripken.github.io/emscripten-site/docs/api_reference/module.html).
  Must follow base path (which may be set to `null` or `undefined`).
- Node-style callback with 2 parameters:
  - Error if present, otherwise `null`.
  - Binary code exports object containing C++ classes.

`nbind` can be initialized synchronously on Node.js and asynchronously on
browsers and Node.js. Purely synchronous is easier but not as future-proof:

```JavaScript
var nbind = require('nbind');
var lib = nbind.init().lib;

// Use the library.
```

Using a callback also supports asynchronous initialization:

```JavaScript
var nbind = require('nbind');

nbind.init(function(err, binding) {
  var lib = binding.lib;

  // Use the library.
});
```

The callback passed to init currently gets called synchronously in Node.js
and asynchronously in browsers. To avoid releasing
[zalgo](http://blog.izs.me/post/59142742143/designing-apis-for-asynchrony)
you can for example wrap the call in a
[bluebird](http://bluebirdjs.com/docs/api/promise.promisify.html) promise:

```JavaScript
var bluebird = require('bluebird');
var nbind = require('nbind');

bluebird.promisify(nbind.init)().then(function(binding) {
  var lib = binding.lib;

  // Use the library.
});
```

Using nbind headers
-------------------

There are two possible files to include:

- `nbind/api.h` for using types from the `nbind` namespace such as JavaScript callbacks inside your C++ code.
  - `#include` **before** your own class definitions.
  - Causes your code to depend on `nbind`.
- `nbind/nbind.h` for exposing your C++ API to JavaScript.
  - `#include` **after** your own class definitions to avoid accidentally invoking its macros.
  - The header automatically hides itself if not targeting Node.js or asm.js.
  - Safe to use in any projects.

Use `#include "nbind/nbind.h"` at the end of your source file with only the bindings after it.
The header defines macros with names like `construct` and `method` that may otherwise break
your code or conflict with other headers.

It's OK to include `nbind/nbind.h` also when not targeting any JavaScript environment.
`node-gyp` defines a `BUILDING_NODE_EXTENSION` macro and Emscripten defines an `EMSCRIPTEN` macro
so when those are undefined, the include file does nothing.

Use `#include "nbind/api.h"` in your header files to use types in the nbind namespace
if you need to [report errors](#error-handling) without throwing exceptions,
or want to pass around [callbacks](#callbacks) or [objects](#using-objects).

You can use an `#ifdef NBIND_CLASS` guard to skip your `nbind` export definitions when the headers weren't loaded.

Example that uses an `nbind` callback in C++ code:

**[`1-headers.cc`](https://raw.githubusercontent.com/charto/nbind-examples/master/1-headers.cc)**

```C++
#include <string>
#include <iostream>

// For nbind::cbFunction type.
#include "nbind/api.h"

class HeaderExample {

public:

  static void callJS(nbind::cbFunction &callback) {
    std::cout << "JS says: " << callback.call<std::string>(1, 2, 3);
  }

};

// For NBIND_CLASS() and method() macros.
#include "nbind/nbind.h"

#ifdef NBIND_CLASS

NBIND_CLASS(HeaderExample) {
  method(callJS);
}

#endif
```

Example used from JavaScript:

**[`1-headers.js`](https://raw.githubusercontent.com/charto/nbind-examples/master/1-headers.js)**

```JavaScript
var nbind = require('nbind');

var lib = nbind.init().lib;

lib.HeaderExample.callJS(function(a, b, c) {
  return('sum = ' + (a + b + c) + '\n');
});
```

Run the example with `node 1-headers.js` after [installing](#installing-the-examples). It prints:

```
JS says: sum = 6
```

Classes and constructors
------------------------

The `NBIND_CLASS(className)` macro takes the name of your C++ class as an argument (without any quotation marks), and exports it to JavaScript using the same name. It's followed by a curly brace enclosed block of method exports, as if it was a function definition.

Constructors are exported with a macro call `construct<types...>();` where `types` is a comma-separated list of arguments to the constructor, such as `int, int`. Calling `construct` multiple times allows overloading it, but **each overload must have a different number of arguments**.

Constructor arguments are the only types that `nbind` cannot detect automatically.

Example with different constructor argument counts and types:

**[`2-classes.cc`](https://raw.githubusercontent.com/charto/nbind-examples/master/2-classes.cc)**

```C++
#include <iostream>

class ClassExample {

public:

  ClassExample() {
    std::cout << "No arguments\n";
  }
  ClassExample(int a, int b) {
    std::cout << "Ints: " << a << " " << b << "\n";
  }
  ClassExample(const char *msg) {
    std::cout << "String: " << msg << "\n";
  }

};

#include "nbind/nbind.h"

NBIND_CLASS(ClassExample) {
  construct<>();
  construct<int, int>();
  construct<const char *>();
}
```

Example used from JavaScript:

**[`2-classes.js`](https://raw.githubusercontent.com/charto/nbind-examples/master/2-classes.js)**

```JavaScript
var nbind = require('nbind');

var lib = nbind.init().lib;

var a = new lib.ClassExample();
var b = new lib.ClassExample(42, 54);
var c = new lib.ClassExample("Don't panic");
```

Run the example with `node 2-classes.js` after [installing](#installing-the-examples). It prints:

```
No arguments
Ints: 42 54
String: Don't panic
```

Methods and properties
----------------------

Methods are exported inside an `NBIND_CLASS` block with a macro call `method(methodName);`
which takes the name of the method as an argument (without any quotation marks).
The C++ method gets exported to JavaScript with the same name.

Properties should be accessed through [getter and setter functions](#getters-and-setters).

Data types of method arguments and its return value are detected automatically
so you don't have to specify them. Note the [supported data types](#type-conversion)
because using other types may cause compiler errors that are difficult to understand.

If the method is `static`, it becomes a property of the JavaScript constructor function
and can be accessed like `className.methodName()`. Otherwise it becomes a property of
the prototype and can be accessed like `obj = new className(); obj.methodName();`

Example with a method that counts a cumulative checksum of ASCII character values in strings,
and a static method that processes an entire array of strings:

**[`3-methods.cc`](https://raw.githubusercontent.com/charto/nbind-examples/master/3-methods.cc)**

```C++
#include <string>
#include <vector>

class MethodExample {

public:

  unsigned int add(std::string part) {
    for(char &c : part) sum += c;

    return(sum);
  }

  static std::vector<unsigned int> check(std::vector<std::string> list) {
    std::vector<unsigned int> result;
    MethodExample example;

    for(auto &&part : list) result.push_back(example.add(part));

    return(result);
  }

  unsigned int sum = 0;

};

#include "nbind/nbind.h"

NBIND_CLASS(MethodExample) {
  construct<>();

  method(add);
  method(check);
}
```

Example used from JavaScript, first calling a method in a loop from JS
and then a static method returning an array:

**[`3-methods.js`](https://raw.githubusercontent.com/charto/nbind-examples/master/3-methods.js)**

```JavaScript
var nbind = require('nbind');

var lib = nbind.init().lib;

var parts = ['foo', 'bar', 'quux'];

var checker = new lib.MethodExample();

console.log(parts.map(function(part) {
  return(checker.add(part));
}));

console.log(lib.MethodExample.check(parts));
```

Run the example with `node 3-methods.js` after [installing](#installing-the-examples). It prints:

```
[ 324, 633, 1100 ]
[ 324, 633, 1100 ]
```

The example serves to illustrate passing data.
In practice, such simple calculations are faster to do in JavaScript
rather than calling across languages because copying data is quite expensive.

Getters and setters
-------------------

Property getters are exported inside an `NBIND_CLASS` block with a macro call
`getter(getterName)` with the name of the getter method as an argument.
`nbind` automatically strips a `get`/`Get`/`get_`/`Get_` prefix and
converts the next letter to lowercase, so for example `getX` and `get_x`
both would become getters of `x` to be accessed like `obj.x`

Property setters are exported together with getters using a macro call
`getset(getterName, setterName)` which works much like `getter(getterName)` above.
Both `getterName` and `setterName` are mangled individually so
you can pair `getX` with `set_x` if you like.
From JavaScript, `++obj.x` would then call both of them to read and change the property.

Example class and property with a getter and setter:

**[`4-getset.cc`](https://raw.githubusercontent.com/charto/nbind-examples/master/4-getset.cc)**

```C++
class GetSetExample {

public:

  void setValue(int value) { this->value = value; }
  int getValue() { return(value); }

private:

  int value = 42;

};

#include "nbind/nbind.h"

NBIND_CLASS(GetSetExample) {
  construct<>();

  getset(getValue, setValue);
}
```

Example used from JavaScript:

**[`4-getset.js`](https://raw.githubusercontent.com/charto/nbind-examples/master/4-getset.js)**

```JavaScript
var nbind = require('nbind');

var lib = nbind.init().lib;

var obj = new lib.GetSetExample();

console.log(obj.value++); // 42
console.log(obj.value++); // 43
```

Run the example with `node 4-getset.js` after [installing](#installing-the-examples).

Passing data structures
-----------------------

`nbind` supports automatically converting between JavaScript arrays and C++
`std::vector` or `std::array` types. Just use them as arguments or return values
in C++ methods.

Note that data structures don't use the same memory layout in both languages,
so the data always gets copied which takes more time for more data.
For example the strings in an array of strings also get copied,
one character at a time. In asm.js data is copied twice, first to a temporary
space using a common format both languages can read and write.

Callbacks
---------

Callbacks can be passed to C++ methods by simply adding an argument of type
`nbind::cbFunction &` to their declaration.

They can be called with any number of any supported types without having to declare in any way what they accept.
The JavaScript code will receive the parameters as JavaScript variables to do with them as it pleases.

A callback argument `arg` can be called like `arg("foobar", 42);` in which case the return value is ignored.
If the return value is needed, the callback must be called like `arg.call<type>("foobar", 42);`
where type is the desired C++ type that the return value should be converted to.
This is because the C++ compiler cannot otherwise know what the callback might return.

Warning: currently callbacks have a very short lifetime!
They can be called only until the first function that received them returns.
That means it's possible to create a function like `Array.map`
which calls a callback zero or more times and then returns, never using the callback again.
It's currently not possible to create a function like `setTimeout`
which calls the callback after it has returned.

Using objects
-------------

C++ objects can be passed to and from JavaScript *by reference* using pointers
or *by value* using objects as parameters and return values in C++ code.

Note: currently passing objects by pointer on Node.js requires the class
to have a "copy constructor" initializing itself from a pointer.
This will probably be fixed later.

Using pointers is particularly:

- **dangerous** because the pointer may become invalid
  without JavaScript noticing it.
- **annoying** in asm.js because browsers give no access to the garbage collector,
  so memory may leak when pointers become garbage without C++ noticing it.
  Smart pointers are not supported until a workaround for this comes up.

Passing data by value using *value objects* solves both issues.
They're based on a `toJS` function on the C++ side
and a `fromJS` function on the JavaScript side.
Both receive a callback as an argument, and calling it with any parameters
calls the constructor of the equivalent type in the other language.

The callback on the C++ side is of type `nbind::cbOutput`.
Value objects are passed through the C++ stack to and from the exported function.
`nbind` uses C++11 move semantics to avoid creating some additional copies on the way.

The equivalent JavaScript constructor must be registered on the JavaScript side
by calling `binding.bind('CppClassName', JSClassName)`
so that `nbind` knows which types to translate between each other.

Example with a class `Coord` used as a value object, and a class
`ObjectExample` which uses objects passed by values and references:

**[`5-objects.cc`](https://raw.githubusercontent.com/charto/nbind-examples/master/5-objects.cc)**

```C++
#include <iostream>

#include "nbind/api.h"

class Coord {

public:

  Coord(signed int x = 0, signed int y = 0) : x(x), y(y) {}
  explicit Coord(const Coord *other) : x(other->x), y(other->y) {}

  void toJS(nbind::cbOutput output) {
    output(x, y);
  }

  signed int getX() { std::cout << "Get X\n"; return(x); }
  signed int getY() { std::cout << "Get Y\n"; return(y); }

  void setX(signed int x) { this->x = x; }
  void setY(signed int y) { this->y = y; }

  signed int x, y;

};

class ObjectExample {

public:

  static void showByValue(Coord coord) {
    std::cout << "C++ value " << coord.x << ", " << coord.y << "\n";
  }

  static void showByRef(Coord *coord) {
    std::cout << "C++ ref " << coord->x << ", " << coord->y << "\n";
  }

  static Coord getValue() {
    return(Coord(12, 34));
  }

  static Coord *getRef() {
    static Coord coord(56, 78);
    return(&coord);
  }

};

#include "nbind/nbind.h"

NBIND_CLASS(Coord) {
  construct<>();
  construct<const Coord *>();
  construct<signed int, signed int>();

  getset(getX, setX);
  getset(getY, setY);
}

NBIND_CLASS(ObjectExample) {
  method(showByValue);
  method(showByRef);
  method(getValue);
  method(getRef);
}
```

Example used from JavaScript:

**[`5-objects.js`](https://raw.githubusercontent.com/charto/nbind-examples/master/5-objects.js)**

```JavaScript
var nbind = require('nbind');

var binding = nbind.init();
var lib = binding.lib;

function Coord(x, y) {
  this.x = x;
  this.y = y;
}

Coord.prototype.fromJS = function(output) {
  output(this.x, this.y);
}

Coord.prototype.show = function() {
  console.log('JS value ' + this.x + ', ' + this.y);
}

binding.bind('Coord', Coord);

var value1 = new Coord(123, 456);
var value2 = lib.ObjectExample.getValue();
var ref = lib.ObjectExample.getRef();

lib.ObjectExample.showByValue(value1);
lib.ObjectExample.showByValue(value2);
value1.show();
value2.show();

lib.ObjectExample.showByRef(ref);
console.log('JS ref ' + ref.x + ', ' + ref.y);
```

Run the example with `node 5-objects.js` after [installing](#installing-the-examples). It prints:

```
C++ value 123, 456
C++ value 12, 34
JS value 123, 456
JS value 12, 34
C++ ref 56, 78
Get X
Get Y
JS ref 56, 78
```

Type conversion
---------------

Parameters and return values of function calls between languages
are automatically converted between equivalent types:

| JavaScript | C++                               |
| ---------- | --------------------------------- |
| number     | (un)signed char, short, int, long |
| number     | float, double                     |
| boolean    | bool                              |
| string     | const (unsigned) char *           |
| string     | std::string                       |
| Array      | std::vector&lt;type&gt;           |
| Array      | std::array&lt;type, size&gt;      |
| Function   | nbind::cbFunction<br>(only as a parameter)<br>See [callbacks](#callbacks) |
| Instance of any prototype<br>(with a fromJS method) | Instance of any class<br>(with a toJS method)<br>See [using objects](#using-objects) |

Error handling
--------------

You can use the `NBIND_ERR("message here");` macro to report an error before returning from C++
(`#include "nbind/api.h"` first). It will be thrown as an error on the JavaScript side
(C++ environments like Emscripten may not support throwing exceptions, but the JavaScript side will).

Publishing on npm
-----------------

Make sure your `package.json` file has at least the required `emcc-path`
and `install` scripts:

```json
  "scripts": {
    "emcc-path": "emcc-path",

    "install": "autogypi && node-gyp configure build"
  }
```

The `dependencies` section should have at least:

```json
  "dependencies": {
    "autogypi": "^0.2.2",
	"nbind": "^0.2.1",
    "node-gyp": "^3.3.1"
  }
```

Your package should also include `binding.gyp` and `autogypi.json` files.

Shipping an asm.js fallback
---------------------------

[nbind-example-universal](https://github.com/charto/nbind-example-universal)
is a good minimal example of compiling a native Node.js addon if possible,
and otherwise using a pre-compiled asm.js version.

It has two temporary build directories `build/native` and `build/asmjs`,
for compiling both versions. `nbind` provides a binary `copyasm`
that can then be used to copy the compiled asm.js library
into a nicer location for publishing inside the final npm package.

Note that the native version should be compiled in the `install` script
so it runs for all users of the package, and the asm.js version should be
compiled in the `prepublish` script so it gets packaged in npm for usage
without the Emscripten compiler. See the
[example `package.json` file](https://github.com/charto/nbind-example-universal/blob/master/package.json).

Using in web browsers
---------------------

[nbind-example-universal](https://github.com/charto/nbind-example-universal)
is a good minimal example also of calling compiled asm.js code from inside
web browsers. The simplest way to get `nbind` working is to add
these scripts in your HTML code as seen in the
[example `index.html`](https://github.com/charto/nbind-example-universal/blob/master/public/index.html):

```html
<script type="text/javascript">
  var Module = {
    onRuntimeInitialized: function() {
      this.ccall('nbind_init');
      var lib = this;

      // Use the library.
    }
  };
</script>

<script src="nbind.js"></script>
```

Make sure to fix the path to `nbind.js` on the last line if necessary.

Using with TypeScript
---------------------

First see [calling from Node.js](#calling-from-nodejs).
Initialization using TypeScript is similar.

Purely synchronous:

```TypeScript
import * as nbind from 'nbind';

const lib = nbind.init<any>().lib;

// Use the library.
```

Asynchronous-aware:

```TypeScript
import * as nbind from 'nbind';

nbind.init((err: any, binding: nbind.Binding<any>) => {
  const lib = binding.lib;

  // Use the library.
});
```

Promise-based:

```TypeScript
import * as bluebird from 'bluebird';
import * as nbind from 'nbind';

bluebird.promisify(nbind.init)().then((binding: nbind.Binding<any>) => {
  const lib = binding.lib;

  // Use the library.
});
```

Note how there is a type argument `<any>` for the init call
in all of the examples. It specifies the contents of `binding.lib` which are
defined in C++ code so the TypeScript compiler cannot guess them.

In a future version `nbind` will also generate a `.ts` file containing an
interface definition for the C++ API. You can then import and use it as the
type argument to get full type checking for API calls from TypeScript.

Debugging
---------

In the browser it can be difficult to stop and debug at the correct spot in
optimized C++ code. `nbind` provides an `_nbind_debug()` function in `api.h`
that you can call from C++ to invoke the browser's debugger when using asm.js.

Authors
=======

- Juha Järvi, befunge<img src="doc/images/gmail.png" alt="domain" width="87" height="16" align="absmiddle">

License
=======

[The MIT License](https://raw.githubusercontent.com/charto/nbind/master/LICENSE)

Copyright (c) 2014-2016 BusFaster Ltd
