//
// Created by rdkgsk on 3/2/26.
//

#ifndef CAD_GL_H
#define CAD_GL_H

#include <QOpenGLFunctions_4_5_Core>

#include "cad_math/vec3.hpp"

void GLSetDefaults();

QOpenGLFunctions_4_5_Core* GL();

static_assert(std::is_same_v<cadm::vec3::Vt, float> || std::is_same_v<cadm::vec3::Vt, double>);
constexpr GLenum GL_CADM_VT_TYPE = std::is_same_v<cadm::vec3::Vt, float>
                                       ? GL_FLOAT
                                       : GL_DOUBLE;
constexpr GLsizei GL_CADM_VT_SIZE = sizeof(cadm::vec3::Vt);

#endif //CAD_GL_H
