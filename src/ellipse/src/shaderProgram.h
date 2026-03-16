//
// Created by rdkgsk on 3/2/26.
//

#ifndef CAD_SHADERPROGRAM_H
#define CAD_SHADERPROGRAM_H

#include "GL.h"
#include <map>
#include <string>

class shaderProgram
{
public:
    explicit shaderProgram();
    ~shaderProgram();

    bool attachShader(GLenum type, const std::string &source);
    bool attachShaderFromFile(GLenum type, std::string filename);
    bool compile();
    void deleteShaders();

    void bind() const;
    void release() const;

    void setUniform1i(const std::string &name, int value) const;

private:
    GLuint m_program;
    std::map<GLenum, GLint> m_shaders;
};


#endif //CAD_SHADERPROGRAM_H