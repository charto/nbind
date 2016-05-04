{
	"conditions": [
		["emcc==1", {
			"make_global_settings": [
				["CXX",  "<!(npm run -s emcc-path)"],
				["LINK", "<!(npm run -s emcc-path)"],
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
