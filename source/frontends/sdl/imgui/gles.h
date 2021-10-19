#pragma once

#if defined(IMGUI_IMPL_OPENGL_ES2)

// Pi3 with Fake KMS
// "OpenGL ES 2.0 Mesa 19.3.2"
// "Supported versions are: 1.00 ES"

#include <GLES2/gl2.h>
#define SA2_CONTEXT_FLAGS 0
#define SA2_CONTEXT_PROFILE_MASK SDL_GL_CONTEXT_PROFILE_ES
#define SA2_CONTEXT_MAJOR_VERSION 2
#define SA2_CONTEXT_MINOR_VERSION 0

 // this is defined in gl2ext.h and nowhere in gl3.h
 #define SA2_IMAGE_FORMAT GL_BGRA_EXT

// this is used in all cases for GL_BGRA_EXT
#include <GLES2/gl2ext.h>

#elif defined(IMGUI_IMPL_OPENGL_ES3)

// Pi4 with Fake KMS
// "OpenGL ES 3.1 Mesa 19.3.2"
// "Supported versions are: 1.00 ES, 3.00 ES, and 3.10 ES"

// On my desktop
// "OpenGL ES 3.1 Mesa 20.2.6"
// "Supported versions are: 1.00 ES, 3.00 ES, and 3.10 ES"

#include <GLES3/gl3.h>
#define SA2_CONTEXT_FLAGS 0
#define SA2_CONTEXT_PROFILE_MASK SDL_GL_CONTEXT_PROFILE_ES
#define SA2_CONTEXT_MAJOR_VERSION 3
#define SA2_CONTEXT_MINOR_VERSION 0

// this is defined in gl2ext.h and nowhere in gl3.h
 #define SA2_IMAGE_FORMAT GL_BGRA_EXT

// "310 es" is accepted on a Pi4, but the imgui shaders do not compile
// this is used in all cases for GL_BGRA_EXT
#include <GLES2/gl2ext.h>

#elif defined(__APPLE__)

#include <SDL_opengl.h>
#define SA2_CONTEXT_FLAGS SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG
#define SA2_CONTEXT_PROFILE_MASK SDL_GL_CONTEXT_PROFILE_CORE
#define SA2_CONTEXT_MAJOR_VERSION 3
#define SA2_CONTEXT_MINOR_VERSION 2

#define SA2_IMAGE_FORMAT GL_RGBA

#else

#include <SDL_opengl.h>
#define SA2_CONTEXT_FLAGS 0
#define SA2_CONTEXT_PROFILE_MASK SDL_GL_CONTEXT_PROFILE_CORE
#define SA2_CONTEXT_MAJOR_VERSION 3
#define SA2_CONTEXT_MINOR_VERSION 2

#define SA2_IMAGE_FORMAT GL_RGBA

#endif


#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include "imgui_memory_editor.h"
