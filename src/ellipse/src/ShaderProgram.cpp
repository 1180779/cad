//
// Created by rdkgsk on 3/2/26.
//

#include <fstream>
#include <QDebug>

#include "ShaderProgram.hpp"

bool ShaderProgram::attachShader(const GLenum type, const std::string &source) {
    const auto gl = getGl();
    const GLuint shader = gl->glCreateShader(type);
    const char *src = source.c_str();
    gl->glShaderSource(shader, 1, &src, nullptr);

    gl->glCompileShader(shader);
    GLint success;
    gl->glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLchar infoLog[512];
        gl->glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        qWarning() << infoLog;
        return false;
    }

    if (m_shaders[type]) {
        const auto previous = m_shaders[type];
        gl->glDeleteShader(previous);
        m_shaders[type] = shader;
    }
    else {
        m_shaders[type] = shader;
    }
    return true;
}

bool ShaderProgram::attachShaderFromFile(GLenum type, std::string filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::string source;
    std::string line;
    while (std::getline(file, line)) {
        source += line + "\n";
    }

    return attachShader(type, source);
}

bool ShaderProgram::compile() {
    const auto gl = getGl();
    m_program = gl->glCreateProgram();
    for (auto &shader : m_shaders) {
        gl->glAttachShader(m_program, shader.second);
    }

    gl->glLinkProgram(m_program);
    GLint success;
    gl->glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        gl->glGetProgramInfoLog(m_program, 512, nullptr, infoLog);
        qWarning() << infoLog;
        return false;
    }
    return true;
}

void ShaderProgram::deleteShaders() {
    const auto gl = getGl();
    for (auto &shader : m_shaders) {
        gl->glDeleteShader(shader.second);
    }
    m_shaders.clear();
}

void ShaderProgram::bind() const {
    const auto gl = getGl();
    gl->glUseProgram(m_program);
}

void ShaderProgram::release() const {
    const auto gl = getGl();
    gl->glUseProgram(0);
}

void ShaderProgram::setUniform1i(const std::string &name, const int value) const {
    const auto gl = getGl();
    const GLint location = gl->glGetUniformLocation(m_program, name.c_str());
    if (location != -1) {
        gl->glUniform1i(location, value);
    }
}

ShaderProgram::ShaderProgram() : m_program(0) {}

ShaderProgram::~ShaderProgram() {
    const auto gl = getGl();
    gl->glDeleteProgram(m_program);
    deleteShaders();
}