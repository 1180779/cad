//
// Created by rdkgsk on 3/2/26.
//

#ifndef CAD_QUAD_H
#define CAD_QUAD_H

#include "GL.h"

class quad
{
public:
    explicit quad();
    ~quad();
    void draw() const;

private:
    GLuint m_vao;
    GLuint m_vbo;
};


#endif //CAD_QUAD_H