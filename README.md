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
Or from a web browser (<a href="#c-in-web-browsers">see below</a>)
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
- Distribute both **native** code and an **asm.js** fallback binary.

In more detail:

- Bind multiple C++ classes, even ones not visible from other files.
- Bind C++ methods simply by mentioning their names.
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

- [Creating your project](#creating-your-project)
- [Configuration](#configuration)
- [Using nbind headers](#using-nbind-headers)
- [Classes and constructors](#classes-and-constructors)
- [Methods and properties](#methods-and-properties)
- [Getters and setters](#getters-and-setters)
- [Arrays and vectors](#arrays-and-vectors)
- [Callbacks](#callbacks)
- [Value types](#value-types)
- [Type conversion](#type-conversion)
- [Error handling](#error-handling)
- [C++ in web browsers](#c-in-web-browsers)

Creating your project
---------------------

TODO

Configuration
----------

TODO

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
or want to pass around callbacks or value objects.

You can use an `#ifdef NBIND_CLASS` guard to skip your `nbind` export definitions when the headers weren't loaded.

Example:

```C++
// For nbind::cbFunction type.
#include "nbind/api.h"

class MyClass {
  static void callJS(nbind::cbFunction &callback);
};

// For NBIND_CLASS() and method() macros.
#include "nbind/nbind.h"

#ifdef NBIND_CLASS

NBIND_CLASS(MyClass) {
  method(callJS);
}

#endif
```

Example used from JavaScript:

```JavaScript
var lib = nbind.init();

lib.MyClass.callJS(function(several, args, here) {
  return(something);
});
```

Classes and constructors
------------------------

The `NBIND_CLASS(className)` macro takes the name of your C++ class as an argument (without any quotation marks), and exports it to JavaScript using the same name. It's followed by a curly brace enclosed block of method exports, as if it was a function definition.

Constructors are exported with a macro call `construct<types...>();` where `types` is a comma-separated list of arguments to the constructor, such as `int, int`. Calling `construct` multiple times allows overloading it, but **each overload must have a different number of arguments**.

Constructor arguments are the only types that `nbind` cannot detect automatically.

Example:

```C++
NBIND_CLASS(MyClass) {
  construct<>();
  construct<int, int>();
  construct<std::string>();
}
```

Example used from JavaScript:

```JavaScript
var lib = nbind.init();

var a = new lib.MyClass();
var b = new lib.MyClass(42, 54);
var c = new lib.MyClass("Don't panic");
```

Methods and properties
----------------------

Methods are exported with a macro call `method(methodName);` which takes the name of the method as an argument (without any quotation marks). It gets exported to JavaScript with the same name. If the method is `static`, it becomes a property of the JavaScript constructor function and can be accessed like `className.methodName()`. Otherwise it becomes a property of the prototype and can be accessed like `obj = new className(); obj.methodName();`

TODO

Getters and setters
-------------------

Property getters are exported with a macro call `getter(getterName)`
which takes the name of the getter method.
`nbind` automatically strips a `get`/`Get`/`get_`/`Get_` prefix and
converts the next letter to lowercase, so for example `getX` and `get_x`
both would become getters of `x` to be accessed like `obj.x`

Property setters are exported together with getters using a macro call
`getset(getterName, setterName)` which works much like `getter(getterName)` above.
Both `getterName` and `setterName` are mangled individually so
you can pair `getX` with `set_x` if you like.
From JavaScript, `++obj.x` would then call both of them to read and change the property.

TODO

Arrays and vectors
------------------

TODO

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

TODO

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
| string     | (const) (unsigned) char *         |
| string     | std::string                       |
| Array      | std::vector&lt;type&gt;           |
| Array      | std::array&lt;type, size&gt;      |
| Function   | nbind::cbFunction<br>(only as a parameter) |
| Instance of any prototype<br>(with a fromJS method) | Instance of any class<br>(with a toJS method) |

Error handling
--------------

You can use the `NBIND_ERR("message here");` macro to report an error before returning from C++
(`#include "nbind/api.h"` first). It will be thrown as an error on the JavaScript side
(C++ environments like Emscripten may not support throwing exceptions, but the JavaScript side will).

C++ in web browsers
-------------------

TODO

License
=======

[The MIT License](https://raw.githubusercontent.com/charto/nbind/master/LICENSE)

Copyright (c) 2014-2016 BusFaster Ltd
