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
    "emcc-path": "emcc-path"
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
- [Using nbind headers](#using-nbind-headers)
- [Classes and constructors](#classes-and-constructors)
- [Methods and properties](#methods-and-properties)
- [Getters and setters](#getters-and-setters)
- [Passing data structures](#passing-data-structures)
- [Callbacks](#callbacks)
- [Value types](#value-types)
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
    "emcc-path": "emcc-path",
    "autogypi": "autogypi",
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
or want to pass around [callbacks](#callbacks) or [value types](#value-types).

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

Value types
-----------

Value objects are perhaps the most advanced concept implemented so far.
They're based on a `toJS` function on the C++ side and a `fromJS` function on the JavaScript side.
Both receive a callback as an argument, and calling it with any parameters
calls the constructor of the equivalent type in the other language
(the bindings use [SFINAE](https://en.wikipedia.org/wiki/Substitution_failure_is_not_an_error)
to detect the toJS function and turn the class into a value type).

The callback on the C++ side is of type `nbind::cbOutput`.
Value objects are passed **by value** on the C++ side to and from the exported function.
`nbind` uses C++11 move semantics to avoid creating some additional copies on the way.

The equivalent JavaScript constructor must be registered on the JavaScript side
by calling `nbind.bind('CppClassName', JSClassName);`,
so `nbind` knows which types to translate between each other.

TODO

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
| Instance of any prototype<br>(with a fromJS method) | Instance of any class<br>(with a toJS method)<br>See [value types](#value-types) |

Error handling
--------------

You can use the `NBIND_ERR("message here");` macro to report an error before returning from C++
(`#include "nbind/api.h"` first). It will be thrown as an error on the JavaScript side
(C++ environments like Emscripten may not support throwing exceptions, but the JavaScript side will).

Publishing on npm
-----------------

TODO

Shipping an asm.js fallback
---------------------------

TODO

Using in web browsers
---------------------

TODO

Using with TypeScript
---------------------

There are two ways to initialize `nbind`, synchronous:

```TypeScript
import * as nbind from 'nbind';

const lib = nbind.init<any>().lib;

// Use the library.
```

and asynchronous-ready:

```TypeScript
import * as nbind from 'nbind';

nbind.init((err: any, binding: nbind.Binding<any>) => {
  const lib = binding.lib;

  // Use the library.
});
```

The callback passed to init currently also gets called synchronously
but in the future some environment might require an async call.
To avoid releasing [zalgo](http://blog.izs.me/post/59142742143/designing-apis-for-asynchrony)
you can for example wrap the call in a [bluebird](http://bluebirdjs.com/docs/api/promise.promisify.html) promise:

```TypeScript
import * as bluebird from 'bluebird';
import * as nbind from 'nbind';

bluebird.promisify(nbind.init)().then((binding: nbind.Binding<any>) => {
  const lib = binding.lib;

  // Use the library.
});
```

Note how somewhere there is a type argument `<any>` for the init call
in all of the examples. It specifies the contents of `binding.lib` which are
defined in C++ code so the TypeScript compiler cannot guess them.

In a future version `nbind` will also generate a `.ts` file containing an
interface definition for the C++ API. You can then import and use it as the
type argument to get full type checking for API calls from TypeScript.

Debugging
---------

TODO

Authors
=======

- Juha Järvi, befunge<img src="doc/images/gmail.png" alt="domain" width="87" height="16" align="absmiddle">

License
=======

[The MIT License](https://raw.githubusercontent.com/charto/nbind/master/LICENSE)

Copyright (c) 2014-2016 BusFaster Ltd
