#include <stdio.h>
#include <stdlib.h>

#include "opengl.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "renderer.h"
#include "shaders.h"
#include "defines.h"
#include "hap.h"

#include "dbg.h"

extern COpenGL * g_openGL;

//######################################
// The message handler for the hidden dummy window
//######################################
LRESULT CALLBACK DLLWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {

	case WM_DESTROY:
		glfwSetWindowShouldClose(g_openGL->m_win, 1); // set close flag
		break;

	case WM_FRAMECHANGED:
		g_openGL->updateTexture((PBYTE)wParam, (size_t)lParam);
		break;

	case WM_SHOW:
		g_openGL->show();//(BOOL)wParam
		break;

	default:
		return CallWindowProc(g_openGL->m_windowProc, hwnd, msg, wParam, lParam);

	}

	return 0;
}

//######################################
//
//######################################
static void error_callback (int error, const char* description) {
	//fprintf(stderr, "Error: %s\n", description);
	ErrorMessage(description);
}

//######################################
//
//######################################
void framebuffer_size_callback (GLFWwindow* window, int width, int height) {
	//NOTE("framebuffer_size_callback")

	glViewport(0, 0, width, height);

	glUniform1i(g_openGL->m_textureUniform, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	//glClearColor(0.0, 0.0, 0.0, 1.0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glfwSwapBuffers(g_openGL->m_win);
}

//######################################
//
//######################################
void window_close_callback (GLFWwindow* window) {
	g_openGL->m_renderer->NotifyEvent(EC_USERABORT, 0, 0);
	glfwSetWindowShouldClose(g_openGL->m_win, 0); // reset the close flag
	glfwHideWindow(g_openGL->m_win);
}

//######################################
//
//######################################
#include <time.h>
static clock_t t_last = 0;
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		clock_t t = clock();
		if (((double)(t - t_last)) * 1000 / CLOCKS_PER_SEC < GetDoubleClickTime()) {
			g_openGL->toggleFullscreen();
			t_last = 0;
		}
		else t_last = t;
	}
}

//######################################
// Constructor
//######################################
COpenGL::COpenGL (CVideoRenderer * renderer) {
	m_renderer = renderer;
}

//######################################
// Destructor
//######################################
COpenGL::~COpenGL() {
	exit();
} 

//######################################
// 
//######################################
int COpenGL::init() {
	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) {
		printf("GLFW initialization failed\n");
		return EXIT_FAILURE;
	}

	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	//glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

	glfwWindowHint(GLFW_DEPTH_BITS, 32);
	glfwWindowHint(GLFW_RED_BITS, 8);
	glfwWindowHint(GLFW_GREEN_BITS, 8);
	glfwWindowHint(GLFW_BLUE_BITS, 8);
	glfwWindowHint(GLFW_ALPHA_BITS, 8);

	m_win = glfwCreateWindow(640, 360, "HapRenderer", NULL, NULL);
	if (!m_win) {
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(m_win);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	// init shaders
	if (!initShaders()) {
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glViewport(0, 0, 640, 360);

	// Disable modes - do we need this?
	glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT0);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	// set callbacks
	//glfwSetFramebufferSizeCallback(m_win, framebuffer_size_callback);
	glfwSetMouseButtonCallback(m_win, mouse_button_callback);
	glfwSetWindowCloseCallback(m_win, window_close_callback);

	// vsync
	glfwSwapInterval(1);

	// override the window proc
	m_hwnd = glfwGetWin32Window(m_win);
	m_windowProc = (WNDPROC)GetWindowLongPtr(m_hwnd, GWLP_WNDPROC);
	SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (LONG_PTR)DLLWindowProc);

	//glfwShowWindow(m_win);
	//glfwSwapBuffers(m_win);

	//glClearColor(0.0, 0.0, 0.0, 1.0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	return 0;
}

//######################################
// 
//######################################
void COpenGL::exit() {
	if (m_win) {
		glfwDestroyWindow(m_win);
		glfwTerminate();
		m_win = NULL;
	}
}

//######################################
// 
//######################################
BOOL COpenGL::initShaders() {
	GLint v, f;
	GLint check = 0;

	// Vertex Shader: step3
	v = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(v, 1, &vertexShader, NULL);
	glCompileShader(v);
	glGetShaderiv(v, GL_COMPILE_STATUS, &check);
	if (check == 0) {
		ErrorMessage("Compiling vertex shader failed");
		return FALSE;
	}

	// Fragment Shader
	f = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(f, 1, &m_fragmentShader, NULL);
	glCompileShader(f);
	glGetShaderiv(f, GL_COMPILE_STATUS, &check);
	if (check == 0) {
		ErrorMessage("Compiling fragment shader failed");
		return FALSE;
	}

	// Program
	GLuint p = glCreateProgram();

	glAttachShader(p, v);
	glAttachShader(p, f);

	glBindAttribLocation(p, ATTRIB_VERTEX, "vertexIn");
	glBindAttribLocation(p, ATTRIB_TEXTURE, "textureIn");

	glLinkProgram(p);

	// Verify link
	glGetProgramiv(p, GL_LINK_STATUS, &check);
	if (check == 0) {
		ErrorMessage("Linking shader program failed");
		return FALSE;
	}

	glUseProgram(p);

	//Get Uniform Variables Location
	m_textureUniform = glGetUniformLocation(p, "tex");

	GLint samplerLoc = -1;
	samplerLoc = glGetUniformLocation(p, "cocgsy_src");
	if (samplerLoc >= 0) {
		glUniform1i(samplerLoc, 0);
	}

	static const GLfloat vertexVertices[] = {
		-1.0f, -1.0f,
		1.0f, -1.0f,
		-1.0f,  1.0f,
		1.0f,  1.0f,
	};

	static const GLfloat textureVertices[] = {
		0.0f,  1.0f,
		1.0f,  1.0f,
		0.0f,  0.0f,
		1.0f,  0.0f,
	};

	glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, vertexVertices);
	glEnableVertexAttribArray(ATTRIB_VERTEX);
	glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);
	glEnableVertexAttribArray(ATTRIB_TEXTURE);

	glGenTextures(1, &m_textureID);
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, m_textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// blue as default?
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		m_textureFormat,
		m_videoSize.cx,
		m_videoSize.cy,
		0,
		GL_BGRA,
		GL_UNSIGNED_INT_8_8_8_8_REV,
		NULL
	);

	return TRUE;
}

//######################################
// 
//######################################
void COpenGL::show() {

	// fill with black
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glfwShowWindow(m_win);
	glfwSwapBuffers(m_win);

	glfwSetFramebufferSizeCallback(m_win, framebuffer_size_callback);
}

//######################################
// 
//######################################
void COpenGL::updateTexture(PBYTE pbData, size_t dataSize) {

	glCompressedTexSubImage2D(
		GL_TEXTURE_2D,
		0,
		0,
		0,
		m_videoSize.cx,
		m_videoSize.cy,
		m_textureFormat,
		(GLsizei)dataSize,
		pbData
	);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glfwSwapBuffers(m_win);
}

//######################################
// 
//######################################
void COpenGL::main() {
	while (!glfwWindowShouldClose(m_win)) {
		glfwPollEvents();
	}
}

//######################################
// 
//######################################
void COpenGL::toggleFullscreen () {
	m_fullScreenFlag = !m_fullScreenFlag;
	if (m_fullScreenFlag) {
		// save current settings		 
		glfwGetWindowSize(m_win, &m_winW, &m_winH);
		glfwGetWindowPos(m_win, &m_winX, &m_winY);

		GLFWmonitor * monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode * mode = glfwGetVideoMode(monitor);
		glfwSetWindowMonitor(
			m_win,
			monitor,
			0,
			0,
			mode->width,
			mode->height,
			mode->refreshRate // GLFW_DONT_CARE
		);
	}
	else {
		glfwSetWindowMonitor(m_win, NULL, m_winX, m_winY, m_winW, m_winH, 0);
	}

	// redraw and sap
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glfwSwapBuffers(m_win);
}

//######################################
// 
//######################################
BOOL COpenGL::setType(VIDEOINFOHEADER *pVideoInfo, const GUID subType) {

	m_videoSize.cx = pVideoInfo->bmiHeader.biWidth;
	m_videoSize.cy = pVideoInfo->bmiHeader.biHeight;

	if (subType == MEDIASUBTYPE_DXT1) {
		m_textureFormat = HapTextureFormat_RGB_DXT1;
		m_fragmentShader = fragmentShader;
	}

	else if (subType == MEDIASUBTYPE_DXT5) {
		m_textureFormat = HapTextureFormat_RGBA_DXT5;
		m_fragmentShader = fragmentShader;
	}

	else if (subType == MEDIASUBTYPE_DXTY) {
		m_textureFormat = HapTextureFormat_RGBA_DXT5;
		m_fragmentShader = fragmentShader_YCoCg_DXT5;
	}

	else if (subType == MEDIASUBTYPE_Hap1) {
		m_textureFormat = HapTextureFormat_RGB_DXT1;
		m_fragmentShader = fragmentShader;
	}

	else if (subType == MEDIASUBTYPE_Hap5) {
		m_textureFormat = HapTextureFormat_RGBA_DXT5;
		m_fragmentShader = fragmentShader;
	}

	else if (subType == MEDIASUBTYPE_HapY) {
		m_textureFormat = HapTextureFormat_RGBA_DXT5;
		m_fragmentShader = fragmentShader_YCoCg_DXT5;
	}

	else return FALSE;
	return TRUE;
}
