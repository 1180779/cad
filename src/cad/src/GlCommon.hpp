//
// Created by rdkgsk on 3/2/26.
//

#ifndef CAD_GL_H
#define CAD_GL_H

#include <QOpenGLFunctions_4_5_Core>
#include "cad_graphics/VertexArray.hxx"

#include "cad_math/Vec3.hxx"

void glSetDefaults();

QOpenGLFunctions_4_5_Core* getGl();

/// Configure CAD's position-only attribute at location 0, borrowing both buffers.
/// Reports a fatal initialization error if creation fails. Requires a current context.
cadg::VertexArray createPositionVertexArray(
    QOpenGLFunctions_4_5_Core& gl, GLuint positionBuffer, GLuint indexBuffer
);

static_assert(std::is_same_v<cadm::Vec3::Vt, float> || std::is_same_v<cadm::Vec3::Vt, double>);
constexpr GLenum gc_glCadmVtType = std::is_same_v<cadm::Vec3::Vt, float>
                                       ? GL_FLOAT
                                       : GL_DOUBLE;
constexpr GLsizei gc_glCadmVtSize = sizeof(cadm::Vec3::Vt);

#endif //CAD_GL_H
