//
// Created by rdkgsk on 3/2/26.
//

#include <QOpenGLContext>
#include <QOpenGLVersionFunctionsFactory>
#include <QDebug>
#include <array>
#include <utility>

#include "GlCommon.hpp"

void glSetDefaults() {
    QSurfaceFormat format;
    format.setVersion(4, 5);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);
}

QOpenGLFunctions_4_5_Core* getGl() {
    QOpenGLVersionProfile profile;
    profile.setVersion(4, 5);
    profile.setProfile(QSurfaceFormat::CoreProfile);

    auto* ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        qWarning() << "GL() called without a current QOpenGLContext!";
        return nullptr;
    }

    auto* gl45 = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_4_5_Core>(ctx);
    if (!gl45) {
        qWarning() << "OpenGL 4.5 Core not supported";
        return nullptr;
    }

    if (!gl45->initializeOpenGLFunctions()) {
        qWarning() << "Failed to initialize OpenGL functions";
        return nullptr;
    }

    return gl45;
}

cadg::VertexArray createPositionVertexArray(
    QOpenGLFunctions_4_5_Core& gl, const GLuint positionBuffer, const GLuint indexBuffer
    ) {
    constexpr std::array attributes{
        cadg::VertexAttribute{
            .location = 0,
            .components = 3,
            .type = gc_glCadmVtType
        },
    };
    const cadg::VertexLayout layout{
        .stride = 3 * gc_glCadmVtSize,
        .attributes = attributes
    };
    auto vertexArray = cadg::VertexArray::create(gl, positionBuffer, layout, indexBuffer);
    if (!vertexArray) {
        qFatal("Failed to create CAD position vertex array: %s", vertexArray.error().c_str());
    }
    return std::move(*vertexArray);
}