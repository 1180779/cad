//
// Created by rdkgsk on 3/2/26.
//

#ifndef CAD_SHADERPROGRAM_H
#define CAD_SHADERPROGRAM_H

#include "GL.h"
#include <map>
#include <string>

class ShaderProgram
{
public:
    explicit ShaderProgram();
    ~ShaderProgram();

    bool attachShader(GLenum type, const std::string& source);
    bool attachShaderFromFile(GLenum type, std::string filename);
    bool compile();
    void deleteShaders();

    void bind() const;
    void release() const;

    void setUniformValue(const std::string& name, int value);

private:
    GLuint m_program;
    std::map<GLenum, GLint> m_shaders;
};


#endif //CAD_SHADERPROGRAM_H