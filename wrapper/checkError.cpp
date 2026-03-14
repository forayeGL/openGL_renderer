#include "checkError.h"
#include <glad/glad.h>
#include <string>
#include <iostream>

void checkError() {
	GLenum errorCode = glGetError();
	if (errorCode == GL_NO_ERROR) {
		return;
	}

	const char* error = "UNKNOWN";
	switch (errorCode) {
		case GL_INVALID_ENUM:                  error = "GL_INVALID_ENUM";                  break;
		case GL_INVALID_VALUE:                 error = "GL_INVALID_VALUE";                 break;
		case GL_INVALID_OPERATION:             error = "GL_INVALID_OPERATION";             break;
		case GL_OUT_OF_MEMORY:                 error = "GL_OUT_OF_MEMORY";                 break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
		default: break;
	}
	std::cerr << "[GL ERROR] " << error << " (0x" << std::hex << errorCode << std::dec << ")" << std::endl;

	// Pause at the exact GL_CALL site so the debugger / RenderDoc call stack
	// shows which OpenGL call generated the error, instead of burying it
	// inside assert()'s own frames.
	__debugbreak();
}