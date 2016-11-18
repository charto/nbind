Tutorial Example of `node` binding to the `vg` library on `MAC OS 10.11` using `g++`.

In order for this example to work, your project structure should look like the following:

    ../git/vg
    ../git/project

- Prearrangements:
    - install `node`
    - set up `vg` according to its instructions https://github.com/vgteam/vg and compile it.
    - `cd` into your `project` folder

Create `JS_VG.cpp` and fill it with whatever functionality and bindings you require. In the following example I am binding an empty constructor and two functions.
```CPP
#include "../vg/src/vg.hpp"
#include "nbind/api.h"

#include "nbind/nbind.h"


namespace vg {

NBIND_CLASS(VG) {

  // Add an empty constructor
  construct<>();

  // Check if the graph is empty
  method(empty);

  // Generate hash out of graph
  method(hash);
}

}
```

Then we have to add relevant scripts to our `package.json', which should look like the following:
```JSON
{
  "scripts": {
    "autogypi": "autogypi",
    "node-gyp": "node-gyp"
  },
  "dependencies": {
    "autogypi": "^0.2.2",
    "nbind": "^0.3.5",
    "node-gyp": "^3.4.0"
  }
}
```
Although, verify that the program versions do match the ones on your machine. I left emscripten out here, because at it's current state it is not able to handle multithreading well enough in order to be applicable to `vg`.
Now install `nbind`, `autogypi` and `node-gyp` locally:

    npm install --save nbind autogypi node-gyp

As `nbind` was designed to work with `clang` on `MAC OS` the following adjustments to the `nbind` module have to be made:

1) Create `g++` under `..git/project/node-modules/nbind/bin/`:
```bash
#!/bin/sh

# This script calls g++ (the homebrew C++ compiler on MAC OS) with given command line
# arguments. It removes the "-stdlib=libc++" flag (and its parameter), which node-gyp
# will add on OS X hosts causing a compiler error.

skip=0

for arg; do
    shift
    [ "$skip" = "1" ] && skip=0 && continue
    case "$arg" in
        -stdlib=*) ;;
        *) set -- "$@" "$arg" ;;
    esac
done

/usr/local/bin/g++ $@
```
Don't forget to make it executable via:

    chmod +x ..git/project/node-modules/nbind/bin/g++

2) Open `..git/project/node-modules/nbind/src/nbind.gypi`. Here comment out `"-stdlib=libc++"` under `OTHER_CPLUSPLUSFLAGS` and the whole line `"OTHER_LDFLAGS": ["-stdlib=libc++"]`.

Now we are ready for:

    npm run -- autogypi --init-gyp -p nbind -s JS_VG.cpp

Among other files a `binding.gyp` is created. Note that we have to point our C++ executable to our `../node_modules/nbind/bin/g++`. This is reflected under `make_global_settings`. Furthermore, we have to add all the `vg` related compilation parameters in the file, finally looking like this:
```JSON
{
"make_global_settings": [
  ["CXX",  "../node_modules/nbind/bin/g++"],
  ["LINK", "../node_modules/nbind/bin/g++"]
],
	"targets": [
		{
			"includes": [
				"auto.gypi"
			],
			"sources": [
				"JS_VG.cpp"
			],
			"conditions": [
				["OS=='mac'", {
					"xcode_settings": {
						"OTHER_CPLUSPLUSFLAGS": [
							"-c",
							"-msse4.1",
							"-fopenmp",
							"-frtti",
							"-ggdb",
							"-g",
							"-I../../vg/",
							"-I../../vg/include"
			    		],
						"OTHER_LDFLAGS": [
							"-msse4.1",
							"-fopenmp",
							"-ggdb",
							"-g",
							"-I../../vg/",
							"-I../../vg/include",
                            "-lvg",
                            "-lsdsl",
                            "-lxg",
                            "-lvcflib",
                            "-lgssw",
                            "-lrocksdb",
                            "-lhts",
                            "-lgcsa2",
                            "-lprotobuf",
                            "-lraptor2",
                            "-lgfakluge",
                            "-lsupbub",
                            "-lpinchesandcacti",
                            "-l3edgeconnected",
                            "-lsonlib",
							"-lm",
							"-lpthread",
							"-ly",
							"-lbz2",
							"-lsnappy",
							"-ldivsufsort",
							"-ldivsufsort64",
							"-ljansson",
							"-L../vg/src",
							"-L../vg/lib"
						]
					},
				}],
			]
		}
	],
	"includes": [
		"auto-top.gypi"
	]
}
```
Feel free to add parameters for any other `OS` under `conditions`.
We are ready for the compilation step now :)

    npm run -- node-gyp configure build

If everything went fine, then create `vg.js` and fill it with:

```JAVASCRIPT
var nbind = require('nbind');
var lib = nbind.init().lib;

var emptyGraph = new lib.VG();

console.log(emptyGraph.hash());
console.log(emptyGraph.empty());
```

Then run it with node:

    node vg.js

And it should print out a hash value and `true`, because the graph is indeed empty.
For your convenience you can find `JS_VG.cpp`, `vg.js`, `binding.gyp` and `package.json`, `g++` and `nbind.gypi` in the following ZIP:
[project.zip](https://github.com/charto/nbind/files/597297/project.zip)

This tutorial is by [subwaystation](https://github.com/subwaystation),
under [Creative Commons Attribution 4.0 International License](http://creativecommons.org/licenses/by/4.0/)
