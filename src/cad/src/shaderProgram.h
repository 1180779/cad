//
// Created by rdkgsk on 3/2/26.
//

#ifndef CAD_SHADERPROGRAM_H
#define CAD_SHADERPROGRAM_H

#include "gl.h"
#include <map>
#include <string>

#include "cad_math/mat4.h"
#include "cad_math/vec2.h"
#include "cad_math/vec3.h"
#include "cad_math/vec4.h"

class shaderProgram
{
public:
    explicit shaderProgram();
    ~shaderProgram();

    bool attachShader(GLenum type, const std::string& source);
    bool attachShaderFromFile(GLenum type, const std::string& filename);
    bool compile();
    void deleteShaders();

    void bind() const;
    void release() const;

    [[nodiscard]] bool setUniform1(const std::string& name, int value) const;
    [[nodiscard]] bool setUniform1(const std::string& name, float value) const;
    [[nodiscard]] bool setUniform2(const std::string& name, float x, float y) const;
    [[nodiscard]] bool setUniform2(const std::string& name, cadm::vec2 vec2) const;
    [[nodiscard]] bool setUniform3(const std::string& name, float x, float y, float z) const;
    [[nodiscard]] bool setUniform3(const std::string& name, const cadm::vec3& vec3) const;
    [[nodiscard]] bool setUniform4(const std::string& name, float x, float y, float z, float w) const;
    [[nodiscard]] bool setUniform4(const std::string& name, const cadm::vec4& vec4) const;
    [[nodiscard]] bool setUniformMat4(const std::string& name, const cadm::mat4& mat4) const;

private:
    template <typename T>
    static constexpr bool isFloatType()
    {
        return std::is_same_v<T, float>;
    }

    template <typename T>
    static constexpr bool isDoubleType()
    {
        return std::is_same_v<T, double>;
    }

    template <typename T>
    bool setUniform1t(const std::string& name, T value) const;

    template <typename T>
    bool setUniform2t(const std::string& name, T x, T y) const;

    template <typename T>
    bool setUniform3t(const std::string& name, T x, T y, T z) const;

    template <typename T>
    bool setUniform4t(const std::string& name, T x, T y, T z, T w) const;

    template <typename T>
    bool setUniformMat4t(const std::string& name, const T& mat) const;

    GLuint m_program;
    std::map<GLenum, GLuint> m_shaders;
};

template <typename T>
bool shaderProgram::setUniform1t(const std::string& name, T value) const
{
    static_assert(isFloatType<T>() || isDoubleType<T>(), "only float and double types are supported");

    const auto gl = GL();
    if (const auto location = gl->glGetUniformLocation(m_program, name.c_str()); location != -1)
    {
        if constexpr (isFloatType<T>())
        {
            gl->glUniform1f(location, value);
        }
        else if constexpr (isDoubleType<T>())
        {
            gl->glUniform1d(location, value);
        }
        return true;
    }
    return false;
}

template <typename T>
bool shaderProgram::setUniform2t(const std::string& name, T x, T y) const
{
    static_assert(isFloatType<T>() || isDoubleType<T>(), "only float and double types are supported");

    const auto gl = GL();
    if (const auto location = gl->glGetUniformLocation(m_program, name.c_str()); location != -1)
    {
        if constexpr (isFloatType<T>())
        {
            gl->glUniform2f(location, x, y);
        }
        else if constexpr (isDoubleType<T>())
        {
            gl->glUniform2d(location, x, y);
        }
        return true;
    }
    return false;
}

template <typename T>
bool shaderProgram::setUniform3t(const std::string& name, T x, T y, T z) const
{
    static_assert(isFloatType<T>() || isDoubleType<T>(), "only float and double types are supported");

    const auto gl = GL();
    if (const auto location = gl->glGetUniformLocation(m_program, name.c_str()); location != -1)
    {
        if constexpr (isFloatType<T>())
        {
            gl->glUniform3f(location, x, y, z);
        }
        else if constexpr (isDoubleType<T>())
        {
            gl->glUniform3d(location, x, y, z);
        }
        return true;
    }
    return false;
}

template <typename T>
bool shaderProgram::setUniform4t(const std::string& name, T x, T y, T z, T w) const
{
    static_assert(isFloatType<T>() || isDoubleType<T>(), "only float and double types are supported");

    const auto gl = GL();
    if (const auto location = gl->glGetUniformLocation(m_program, name.c_str()); location != -1)
    {
        if constexpr (isFloatType<T>())
        {
            gl->glUniform4f(location, x, y, z, w);
        }
        else if constexpr (isDoubleType<T>())
        {
            gl->glUniform4d(location, x, y, z, w);
        }
        return true;
    }
    return false;
}

template <typename T>
bool shaderProgram::setUniformMat4t(const std::string& name, const T& mat) const
{
    using VT = T::VT;
    static_assert(isFloatType<VT>() || isDoubleType<VT>(), "only float and double types are supported");

    const auto gl = GL();
    if (const GLint location = gl->glGetUniformLocation(m_program, name.c_str()); location != -1)
    {
        if constexpr (isFloatType<VT>())
        {
            gl->glUniformMatrix4fv(location, 1, false, static_cast<const GLfloat*>(mat.data));
        }
        else if constexpr (isDoubleType<VT>())
        {
            gl->glUniformMatrix4dv(location, 1, false, static_cast<const GLdouble*>(mat.data));
        }
        return true;
    }
    return false;
}

#endif //CAD_SHADERPROGRAM_H
