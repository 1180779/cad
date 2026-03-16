//
// Created by rdkgsk on 3/2/26.
//

#include <QOpenGLContext>
#include <QOpenGLVersionFunctionsFactory>
#include <QDebug>

#include "GL.h"

void GLSetDefaults()
{
    QSurfaceFormat format;
    format.setVersion(4, 5);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    // format.setDepthBufferSize(24);
    // format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);
}

QOpenGLFunctions_4_5_Core* GL()
{
    QOpenGLVersionProfile profile;
    profile.setVersion(4, 5);
    profile.setProfile(QSurfaceFormat::CoreProfile);

    auto *ctx = QOpenGLContext::currentContext();
    if (!ctx)
    {
        qWarning() << "GL() called without a current QOpenGLContext!";
        return nullptr;
    }

    QOpenGLFunctions_4_5_Core *gl45 = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_4_5_Core>(ctx);
    if (!gl45)
    {
        qWarning() << "OpenGL 4.5 Core not supported";
        return nullptr;
    }

    if (!gl45->initializeOpenGLFunctions())
    {
        qWarning() << "Failed to initialize OpenGL functions";
        return nullptr;
    }

    return gl45;
}