R"({
"chorus": {
	"delay": 30.0,
	"depth": 0.800000011920929,
	"enabled": true,
	"freq": 0.20000000298023224,
	"numVoices": 3.0,
	"width": 27.399999618530273
},
"connections": [
	{
		"dest": 8,
		"destParam": 1,
		"src": 7
	},
	{
		"dest": 0,
		"destParam": 0,
		"src": 2
	},
	{
		"dest": 2,
		"destParam": 2,
		"src": 4
	},
	{
		"dest": 1,
		"destParam": 1,
		"src": 3
	},
	{
		"dest": 2,
		"destParam": 0,
		"src": 1
	},
	{
		"dest": 0,
		"destParam": 0,
		"src": 5
	},
	{
		"dest": 8,
		"destParam": 0,
		"src": 6
	},
	{
		"dest": 5,
		"destParam": 2,
		"src": 8
	}
],
"nodes": [
	{
		"level": 1.0,
		"pan": 0.5,
		"type": "sine",
		"x": 328,
		"y": 232
	},
	{
		"level": 1.0,
		"pan": 0.5,
		"type": "sine",
		"x": 432,
		"y": 232
	},
	{
		"level": 1.0,
		"pan": 0.5,
		"type": "value",
		"value": 4096.0,
		"x": 224,
		"y": 240
	},
	{
		"a": 0.003000000026077032,
		"d": 1.0000001192092896,
		"level": 1.0,
		"pan": 0.5,
		"r": 1.0000001192092896,
		"s": 0.0,
		"type": "adsr",
		"x": 432,
		"y": 312
	},
	{
		"level": 1.0,
		"pan": 0.5,
		"type": "sine",
		"x": 432,
		"y": 152
	},
	{
		"a": 0.003000000026077032,
		"d": 10.0,
		"level": 1.0,
		"pan": 0.5,
		"r": 0.5,
		"s": 0.0,
		"type": "adsr",
		"x": 224,
		"y": 144
	},
	{
		"freq": 0.5,
		"level": 1.0,
		"max": 1.0,
		"min": 0.20000001788139343,
		"pan": 0.5,
		"type": "lfo",
		"x": 224,
		"y": 184
	},
	{
		"level": 1.0,
		"pan": 0.5,
		"type": "mul",
		"x": 328,
		"y": 160
	}
]
})"
