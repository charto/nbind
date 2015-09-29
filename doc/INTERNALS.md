nbind internals
===============

These are some quick notes to better understand (and remember) nbind's internal structure.

Type conversion between C++ and Node.js is defined in `include/nbind/v8/BindingType.h`. It gets loaded from `api.h` and should be included in any code that uses nbind's callbacks. All type converters are needed to fully support `cbFunction::call<ReturnType>` for all possible return types.

Binding definition syntax
-------------------------

Macros to define new bindings are in `BindingShort.h` (due to be renamed, probably to `binding.h`). It contains this gem:

```c++
#define NBIND_CLASS(Name) \
	template<class Bound> struct BindInvoker##Name { \
		BindInvoker##Name(); \
		nbind::BindClass<Name> linkage; \
		nbind::BindDefiner<Name> definer; \
	}; \
	static struct BindInvoker##Name<Name> bindInvoker##Name; \
	template<class Bound> BindInvoker##Name<Bound>::BindInvoker##Name():definer(#Name)
```

It achieves the following:

- Classes are bound with NBIND_CLASS macros, of which any number can be placed anywhere in the program (in the global scope).
- After the program's `main()` is running or the code has been fully dynamically loaded, only a single function call is needed to initialize all bindings. All per-class preparation code runs automatically from constructors of static objects without having to call it.
- A class binding definition looks like a function definition, enclosed in curly braces (it IS a function definition, allowing arbitrary code if needed).
- It's also possible to support Embind-style method call chaining like `class<MyClass>.constructor<...>().function(...).function(...);`
- Binding definitions are not visibly surrounded by any junk.
- The class name needs to be typed only once.

These have the following implications:

- Not repeating the class name requires a macro, because we need a template argument and a string with its name. Only a macro can stringify a type name.
- To run the code before any init function gets run, it must be in the constructor of a static class.
- We need both a class definition and linkage for an instance of it, without name collisions.
- It would be nicer to have nothing but a definition template with the class to bind as a template argument, but a template specialization had issues.
 - What were they? Either there was a name collision, or some extra code was needed after the constructor definition, making its end look uglier (a closing curly brace was not enough)
- A non-static member object is needed to call methods on it for Embind-style method call chaining.

Binding definition handling
---------------------------

Initially all definitions call templated methods in `BindDefiner.h`. They send their template arguments to method, function etc. signatures in the `signature` directory. Their constructors prepare instances of a non-templated `BaseSignature` with a generic `void call()` function pointer. `Caller.h` provides its original definition, which gets cast to a more generic function pointer type (and back later when it actually gets called). Each signature holds a vector of functions with the same argument and return types. The call function must be called with an index to that vector in addition to its actual arguments, so the correct function with an identical signature can be looked up.

Each possible different method or function is represented by a `MethodDef` which contains its name, a `BaseSignature` pointer representing the type of the function and an index to the signature's vector of functions.

The `MethodDef`s are then added to a linked list in `BindClass`.

Node.js bindings
----------------

In Node.js, the JavaScript engine's state can't be touched in the static constructors or it will crash. The code to initialize bindings can only run after v8 calls the `initModule` function (defined in `src/v8/Binding.cc` and also set up with Node.js there by calling the NODE_MODULE macro). This is the reason that bindings definitions store all information in list and vector structures, where they can be read when the engine is ready.

For Node.js, no JavaScript code is needed to bind the C++ classes with it. The init function sets up constructor and prototype templates, and JavaScript call C++ through them.

Emscripten bindings
-------------------

Emscripten would allow calling the JavaScript engine directly from `BindDefiner.h` but we'll set up internal data structures similarly to the Node.js version to keep the implementations similar. This also allows adding some introspection features later.

Since code inside an asm.js block is still JavaScript, C++ and JavaScript functions can call each other directly. However, asm.js code will immediately constrain argument types into something numeric. That means the only job `nbind` has to do is to help with locating the functions to call, and marshall non-numeric types between JavaScript objects and the C++ typed array heap.
