nbind internals
===

Code structure
---

All code that makes `nbind` work is in 2 directories,  
[include/nbind](https://github.com/charto/nbind/tree/master/include/nbind) and
[src](https://github.com/charto/nbind/tree/master/src).
Most platform-dependent code is in separate subdirectories:

- Node.js:
[include/nbind/v8](https://github.com/charto/nbind/tree/master/include/nbind/v8) and [src/v8](https://github.com/charto/nbind/tree/master/src/v8)
- Emscripten:
[include/nbind/em](https://github.com/charto/nbind/tree/master/include/nbind/em) and [src/em](https://github.com/charto/nbind/tree/master/src/em)

Main code paths
---

**Class definition (Bindings for a single C++ class visible to JavaScript)**

- [BindingShort.h](https://github.com/charto/nbind/blob/master/include/nbind/BindingShort.h): `#define NBIND_CLASS`  
Defines a `BindInvoker` class and its static global instance to immediately run a constructor definition that follows the macro.
- [BindClass.h](https://github.com/charto/nbind/blob/master/include/nbind/BindClass.h): `BindClass()`  
Constructs an object that will hold all declared information about the C++ class until an init function is called to simultaneously inform the JavaScript engine about all available bindings.
- [BindDefiner.h](https://github.com/charto/nbind/blob/master/include/nbind/BindDefiner.h): `BindDefiner()`  
Gives the `BindClass` object a name and adds it to a global list. Methods of a `BindDefiner` instance are used inside the `BindInvoker`constructor to define C++ constructors and methods visible in JavaScript.
- [BindingShort.h](https://github.com/charto/nbind/blob/master/include/nbind/BindingShort.h): `BindInvoker()`  
The body of a constructor function, defined in an `NBIND_CLASS` block in the C++ code that uses `nbind`. It gets executed as soon as the library or executable is loaded, before any `main` function.

**Constructor definition**

- [BindingShort.h](https://github.com/charto/nbind/blob/master/include/nbind/BindingShort.h): `#define construct`  
A macro without arguments. Instead it should be followed by template arguments to:
- [BindDefiner.h](https://github.com/charto/nbind/blob/master/include/nbind/BindDefiner.h): `BindDefiner::constructor`  
Method that adds a constructor to the `BindClass`. Types of the C++ class constructor's arguments must be passed as template arguments because they cannot be auto-detected through other template magic.
- [ConstructorSignature.h](https://github.com/charto/nbind/blob/master/include/nbind/signature/ConstructorSignature.h):  `ConstructorSignature()`  
Defines invoker functions to create instances of the C++ class when called from JavaScript.
- [BaseSignature.h](https://github.com/charto/nbind/blob/master/include/nbind/signature/BaseSignature.h):  `BaseSignature()`  
Encapsulates all information about the constructor.
- [BindClass.h](https://github.com/charto/nbind/blob/master/include/nbind/BindClass.h): `BindClass::addConstructor`  
Stores an instance of the `BaseSignature` for initializing the bindings.
