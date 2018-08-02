#pragma once

const char * vertexShader = {
	"attribute vec4 vertexIn;"
	"attribute vec2 textureIn;"
	"varying vec2 textureOut;"
	"void main(void){"
	"	gl_Position = vertexIn;"
	"	textureOut = textureIn;"
	"}"
};

const char * fragmentShader = {
	"varying vec2 textureOut;"
	"uniform sampler2D tex;"
	"void main(void){"
	"	gl_FragColor = texture2D(tex, textureOut);"
	"}"
};

const char * fragmentShader_YCoCg_DXT5 = {
	"varying vec2 textureOut;"
	"uniform sampler2D cocgsy_src;"
	"const vec4 offsets = vec4(-0.50196078431373, -0.50196078431373, 0.0, 0.0);"
	"void main(){"
	"	vec4 CoCgSY = texture2D(cocgsy_src, textureOut);"
	"	CoCgSY += offsets;"
	"	float scale = (CoCgSY.z * (255.0 / 8.0)) + 1.0;"
	"	float Co = CoCgSY.x / scale;"
	"	float Cg = CoCgSY.y / scale;"
	"	float Y = CoCgSY.w;"
	"	vec4 rgba = vec4(Y + Co - Cg, Y + Cg, Y - Co - Cg, 1.0);"
	"	gl_FragColor = rgba;"
	"}"
};
