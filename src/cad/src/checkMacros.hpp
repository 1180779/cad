//
// Created on 3/15/26.
//

#ifndef CAD_CHECKMACROS_HPP
#define CAD_CHECKMACROS_HPP

#include <QtLogging>

#include "gl.h"

inline const char* GLErrorToString(const GLenum error)
{
    switch (error)
    {
        case GL_INVALID_ENUM:
            return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:
            return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION:
            return "GL_INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY:
            return "GL_OUT_OF_MEMORY";
        default:
            return "UNKNOWN_GL_ERROR";
    }
}

inline bool LogGLErrorsIfAny(const char* file, const int line)
{
    bool hasError = false;
    const auto gl = GL();
    if (!gl)
    {
        return false;
    }

    for (GLenum error = gl->glGetError(); error != GL_NO_ERROR; error = gl->glGetError())
    {
        hasError = true;
        qWarning() << "OpenGL error" << GLErrorToString(error) << "(" << static_cast<unsigned int>(error) << ") at"
                   << file << "," << line;
    }

    return hasError;
}

#define UNIQUE_PTR_RELEASE_CHECK(expr) \
    do { \
        if(!expr) { \
            qWarning() << "Unique ptr release failed" << __FILE__ << ", " << __LINE__; \
        } \
    } while(0)

#define GET_GL_ERRORS() \
    do { \
        LogGLErrorsIfAny(__FILE__, __LINE__); \
    } while(0)

#define SHADER_SET_UNIFORM_CHECK(expr) \
    do { \
        if(!expr) { \
            qWarning() << "Shader set uniform failed" << __FILE__ << ", " << __LINE__; \
            GET_GL_ERRORS(); \
        } \
    } while (0)

#define SHADER_ATTACHING_CHECK(expr) \
    do { \
        if (!expr) { \
            qWarning() << "Shader compilation failed: " << __FILE__ << ", " << __LINE__; \
            GET_GL_ERRORS(); \
        } \
    } while (0)

#define SHADER_COMPILATION_CHECK(expr) \
    do { \
        if (!expr) { \
            qWarning() << "Shader compilation failed: " << __FILE__ << ", " << __LINE__; \
            GET_GL_ERRORS(); \
        } \
    } while (0)


#endif //CAD_CHECKMACROS_HPP
