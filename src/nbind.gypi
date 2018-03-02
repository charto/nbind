{

	"variables": {
		"asmjs%": 0
	},

	"target_name": "nbind",
	"type": "loadable_module",
	"sources": [
		"common.cc",
		"reflect.cc"
	],
	"include_dirs": [
		"../include"
	],

	"conditions": [
		['asmjs==1', {

			"product_name": "nbind.js",
			"type":         "executable",
			"sources":    [ "em/Binding.cc" ],
			"ldflags":    [ "<@(_cflags)" ],

			"copies": [{"destination": "<(INTERMEDIATE_DIR)", "files": ["pre.js", "post.js", "../dist/em-api.js"]}],
			"prejs_path": "<(INTERMEDIATE_DIR)/pre.js",
			"postjs_path": "<(INTERMEDIATE_DIR)/post.js",
			"jslib_path": "<(INTERMEDIATE_DIR)/em-api.js",

			"cflags": [
				"-O3",
				"--pre-js", "<(_prejs_path)",
				"--post-js", "<(_postjs_path)",
				"--js-library", "<(_jslib_path)",
				"-s", "NO_FILESYSTEM=1",
				"-s", "EXPORTED_FUNCTIONS=[\"_nbind_init\",\"_nbind_value\"]",
				"-s", "DEFAULT_LIBRARY_FUNCS_TO_INCLUDE=[\"nbind_value\",\"\\$$Browser\"]"
			],

			"cflags_cc": [
				"-std=c++11",
				"-fno-exceptions"
			],

			"xcode_settings": {
				"GCC_GENERATE_DEBUGGING_SYMBOLS": "NO",
				"OTHER_CFLAGS": [ "<@(_cflags)" ],
				"OTHER_CPLUSPLUSFLAGS": [ "<@(_cflags_cc)" ],
				"OTHER_LDFLAGS": [ "<@(_cflags)" ]
			}

		}, {

			"copies": [{"destination": "<(INTERMEDIATE_DIR)", "files": ["symbols.txt"]}],
			"symbols_path": "<(INTERMEDIATE_DIR)/symbols.txt",

			"sources": [
				"v8/Buffer.cc",
				"v8/Binding.cc"
			],

			"cflags": [
				"-O3",
				"-fPIC"
			],

			"cflags_cc": [
				"-std=c++11",
				"-fexceptions",
				"-fPIC"
			],

			"msbuild_settings": {
				"ClCompile": {
					"RuntimeTypeInfo": "false",
					"ExceptionHandling": "Sync", # /EHsc
					"MultiProcessorCompilation": "true"
				}
			},

			"xcode_settings": {
				"GCC_ENABLE_CPP_EXCEPTIONS": "YES",
				"CLANG_CXX_LANGUAGE_STANDARD": "c++11",
				"MACOSX_DEPLOYMENT_TARGET": "10.7",
				"OTHER_CFLAGS": [ "<@(_cflags)" ],
				"OTHER_CPLUSPLUSFLAGS": [
					"<@(_cflags_cc)",
					"-stdlib=libc++"
				],
				"OTHER_LDFLAGS": [
					"-stdlib=libc++",
					"-exported_symbols_list", "<(_symbols_path)"
				]
			}

		}]
	]
}
