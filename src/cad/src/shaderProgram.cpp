//
// Created by rdkgsk on 3/2/26.
//

#include <fstream>
#include <QDebug>

#include "shaderProgram.h"

shaderProgram::shaderProgram()
    : m_program(0)
{
}

shaderProgram::~shaderProgram()
{
    const auto gl = GL();
    gl->glDeleteProgram(m_program);
    deleteShaders();
}

bool shaderProgram::attachShader(const GLenum type, const std::string& source)
{
    const auto gl = GL();
    const GLuint shader = gl->glCreateShader(type);
    const char* src = source.c_str();
    gl->glShaderSource(shader, 1, &src, nullptr);

    gl->glCompileShader(shader);
    GLint success;
    gl->glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        GLchar infoLog[512];
        gl->glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        qWarning() << "Shader compilation error:" << infoLog;
        return false;
    }

    if (m_shaders[type])
    {
        const auto previous = m_shaders[type];
        gl->glDeleteShader(previous);
        m_shaders[type] = shader;
    }
    else
    {
        m_shaders[type] = shader;
    }
    return true;
}

bool shaderProgram::attachShaderFromFile(const GLenum type, const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        qWarning() << "Failed to open shader file:" << filename.c_str(); // Added warning here
        return false;
    }

    std::string source;
    std::string line;
    while (std::getline(file, line))
    {
        source += line + "\n";
    }

    return attachShader(type, source);
}

bool shaderProgram::compile()
{
    const auto gl = GL();
    m_program = gl->glCreateProgram();
    for (const auto& val : m_shaders | std::views::values)
    {
        gl->glAttachShader(m_program, val);
    }

    gl->glLinkProgram(m_program);
    GLint success;
    gl->glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[512];
        gl->glGetProgramInfoLog(m_program, 512, nullptr, infoLog);
        qWarning() << "Shader linking error:" << infoLog;
        return false;
    }
    return true;
}

void shaderProgram::deleteShaders()
{
    const auto gl = GL();
    for (const auto& val : m_shaders | std::views::values)
    {
        gl->glDeleteShader(val);
    }
    m_shaders.clear();
}

void shaderProgram::bind() const
{
    const auto gl = GL();
    gl->glUseProgram(m_program);
}

void shaderProgram::release() const
{
    const auto gl = GL();
    gl->glUseProgram(0);
}

bool shaderProgram::setUniform1(const std::string& name, const int value) const
{
    const auto gl = GL();
    if (const GLint location = gl->glGetUniformLocation(m_program, name.c_str()); location != -1)
    {
        gl->glUniform1i(location, value);
        return true;
    }
    return false;
}

bool shaderProgram::setUniform1(const std::string& name, const float value) const
{
    return setUniform1t(name, value);
}

bool shaderProgram::setUniform2(const std::string& name, const float x, const float y) const
{
    return setUniform2t(name, x, y);
}

bool shaderProgram::setUniform2(const std::string& name, const cadm::vec2 vec2) const
{
    return setUniform2t(name, vec2.x, vec2.y);
}

bool shaderProgram::setUniform3(const std::string& name, const float x, const float y, const float z) const
{
    return setUniform3t(name, x, y, z);
}

bool shaderProgram::setUniform3(const std::string& name, const cadm::vec3& vec3) const
{
    return setUniform3t(name, vec3.x, vec3.y, vec3.z);
}

bool shaderProgram::setUniform4(
    const std::string& name, const float x, const float y, const float z, const float w) const
{
    return setUniform4t(name, x, y, z, w);
}

bool shaderProgram::setUniform4(const std::string& name, const cadm::vec4& vec4) const
{
    return setUniform4t(name, vec4.x, vec4.y, vec4.z, vec4.w);
}

bool shaderProgram::setUniformMat4(const std::string& name, const cadm::mat4& mat4) const
{
    return setUniformMat4t(name, mat4);
}
