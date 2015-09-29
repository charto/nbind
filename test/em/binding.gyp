{
	"conditions": [
		['emcc==1', {
			"make_global_settings": [
				["CXX",  "<!(which emcc)"],
				["LINK", "<!(which emcc)"],
			]
		}]
	],

	"targets": [
		{
			"includes": [
				"auto.gypi",
				"../test.gypi"
			]
		}
	]
}
