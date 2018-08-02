#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "renderer.h"

class CVideoRenderer;

class COpenGL
{

public:
	CVideoRenderer * m_renderer;
	HWND m_hwnd = NULL;
	GLFWwindow * m_win = NULL;
	const char * m_fragmentShader = NULL;
	int m_textureFormat;
	SIZE m_videoSize;
	GLuint m_textureID;
	GLuint m_textureUniform;
	WNDPROC m_windowProc;
	BOOL m_fullScreenFlag = FALSE;

	// used for returning from fullscreen
	int m_winX, m_winY, m_winW, m_winH;

public:
	COpenGL(CVideoRenderer*);
	virtual ~COpenGL();

	int init();
	void exit();

	BOOL initShaders();
	void updateTexture(PBYTE pbData, size_t dataSize);
	void main();
	void show();
	void toggleFullscreen();

	BOOL setType (VIDEOINFOHEADER *pVideoInfo, const GUID);
};
