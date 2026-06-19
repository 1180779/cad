//
// Created by rdkgsk on 3/2/26.
//

#ifndef CAD_GL_H
#define CAD_GL_H

#include <QOpenGLFunctions_4_5_Core>

#include "cad_math/vec3.hpp"

void glSetDefaults();

QOpenGLFunctions_4_5_Core* getGl();

static_assert(std::is_same_v<cadm::vec3::Vt, float> || std::is_same_v<cadm::vec3::Vt, double>);
constexpr GLenum gc_glCadmVtType = std::is_same_v<cadm::vec3::Vt, float>
                                       ? GL_FLOAT
                                       : GL_DOUBLE;
constexpr GLsizei gc_glCadmVtSize = sizeof(cadm::vec3::Vt);

#endif //CAD_GL_H
