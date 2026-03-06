//
// Created by rdkgsk on 3/1/26.
//

#include "OpenGLWidget.h"

#include "cad_math/helpers.h"
#include "cad_math/mat4.h"

OpenGLWidget::OpenGLWidget(QWidget* parent) : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

OpenGLWidget::~OpenGLWidget() = default;

void OpenGLWidget::paintGL()
{
    if (m_currentAdaptationStep <= m_renderState.adaptationSize)
    {
        performRaycasting(m_renderState, m_cpuBuffer, m_currentAdaptationStep);

        const auto gl = GL();
        gl->glBindTexture(GL_TEXTURE_2D, m_texture);
        gl->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width(), height(), GL_RGB, GL_UNSIGNED_BYTE, m_cpuBuffer.data());

        m_currentAdaptationStep++;
        if (m_currentAdaptationStep <= m_renderState.adaptationSize)
        {
            update();
        }
    }

    const auto gl = GL();
    gl->glClear(GL_COLOR_BUFFER_BIT);
    m_shaderProgram->bind();
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, m_texture);
    m_shaderProgram->setUniformValue("screenTexture", 0);
    m_quad->draw();
    m_shaderProgram->release();
}

void OpenGLWidget::resizeGL(const int width, const int height)
{
    QOpenGLWidget::resizeGL(width, height);
    const size_t bufferSize = width * height * 3;
    m_cpuBuffer.resize(bufferSize);

    const auto gl = GL();
    gl->glBindTexture(GL_TEXTURE_2D, m_texture);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    m_camera.setAspectRatio(static_cast<cadm::cadf>(width) / static_cast<cadm::cadf>(height));

    updateRenderParams();

    // qInfo() << "Resized to " << width << "x" << height;
}

void OpenGLWidget::initializeGL()
{
    const auto gl = GL();
    gl->glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // for rgb the cpu buffer is not aligned, which leads to artifacts
    gl->glClearColor(0.0f, 1.0f, 1.0f, 1.0f);

    m_quad = std::make_unique<quad>();

    m_shaderProgram = std::make_unique<ShaderProgram>();
    m_shaderProgram->attachShaderFromFile(GL_VERTEX_SHADER, "shaders/vertexShader.vert");
    m_shaderProgram->attachShaderFromFile(GL_FRAGMENT_SHADER, "shaders/fragmentShader.frag");
    m_shaderProgram->compile();

    // create texture
    gl->glGenTextures(1, &m_texture);
    gl->glBindTexture(GL_TEXTURE_2D, m_texture);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width(), height(), 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void OpenGLWidget::setA(const cadm::cadf a)
{
    if (m_a == a)
        return;
    m_a = a;
    updateRenderParams();
}

void OpenGLWidget::setB(const cadm::cadf b)
{
    if (m_b == b)
        return;
    m_b = b;
    updateRenderParams();
}

void OpenGLWidget::setC(const cadm::cadf c)
{
    if (m_c == c)
        return;
    m_c = c;
    updateRenderParams();
}

void OpenGLWidget::setTranslation(const cadm::vec3 translation)
{
    if (m_translation == translation)
        return;
    m_translation = translation;
    updateRenderParams();
}

void OpenGLWidget::setRotation(const cadm::vec3 rotation)
{
    if (m_rotation == rotation)
        return;
    m_rotation = rotation;
    updateRenderParams();
}

void OpenGLWidget::setScale(const cadm::vec3 scale)
{
    if (m_scale == scale)
        return;
    m_scale = scale;
    updateRenderParams();
}

void OpenGLWidget::setAdaptationSize(const unsigned char adaptationSize)
{
    m_adaptationSize = adaptationSize;
    updateRenderParams();
}

void OpenGLWidget::setM(const cadm::cadf m)
{
    if (m_m == m)
        return;
    m_m = m;
    updateRenderParams();
}

void OpenGLWidget::setAmbientR(const int r)
{
    if (m_ambient.r == r)
        return;
    m_ambient.r = r;
    updateRenderParams();
}

void OpenGLWidget::setAmbientG(const int g)
{
    if (m_ambient.g == g)
        return;
    m_ambient.g = g;
    updateRenderParams();
}

void OpenGLWidget::setAmbientB(const int b)
{
    if (m_ambient.b == b)
        return;
    m_ambient.b = b;
    updateRenderParams();
}

void OpenGLWidget::updateRenderParams()
{
    m_renderState.width = width();
    m_renderState.height = height();
    m_renderState.a = m_a;
    m_renderState.b = m_b;
    m_renderState.c = m_c;
    m_renderState.m = m_m;
    m_renderState.ambient = m_ambient;
    m_renderState.specularColor = m_specularColor;
    m_renderState.adaptationSize = m_adaptationSize;
    m_renderState.cameraPos = m_camera.getPosition();

    const auto translationM = cadm::mat4::translation(m_translation);
    const auto rotationM = cadm::mat4::rotX(m_rotation.x) * cadm::mat4::rotY(m_rotation.y) *
        cadm::mat4::rotZ(m_rotation.z);
    const auto scaleM = cadm::mat4::scale(m_scale);
    const auto M = translationM * rotationM * scaleM;

    const auto viewM = m_camera.getViewMatrix();
    const auto D = cadm::mat4::diag(1.0 / (m_a * m_a), 1.0 / (m_b * m_b), 1.0 / (m_c * m_c), -1.0);
    m_renderState.Minv = M.inverse();
    m_renderState.MinvT = m_renderState.Minv.transposed();
    m_renderState.Dprim = m_renderState.MinvT * D * m_renderState.Minv;
    m_renderState.invPV = (m_camera.getProjectionMatrix() * viewM).inverse();

    m_currentAdaptationStep = 1;
    update();
}

void OpenGLWidget::performRaycasting(const RenderState& state, std::vector<unsigned char>& buffer, int adaptationStep)
{
    const int w = state.width;
    const int h = state.height;

    const int step = std::max(1, static_cast<int>(state.adaptationSize) - adaptationStep + 1);

    for (int py = 0; py < h; py += step)
    {
        for (int px = 0; px < w; px += step)
        {
            // (O + t*Dir)^T * D' (O + t*Dir) = 0
            // ...
            // O^T * D * O + t(O^T * D' * Dir + Dir^T * D' * O) + t^2* Dir^T * D' * Dir
            //
            // a = P^T * D' * Dir
            // b = O^T * D' * Dir + Dir^T * D' * O = 2 * O^T * D' * Dir
            // c = O^T * D * O

            cadm::ray4 rayWorld = cadm::unprojectRay(cadm::vec2i(px, py), -1.0, state.invPV, w, h);

            const auto DprimDir = state.Dprim * rayWorld.direction;
            const auto DprimO = state.Dprim * rayWorld.origin;

            const auto a = rayWorld.direction.dot(DprimDir);
            const auto b = 2.0f * rayWorld.origin.dot(DprimDir);
            const auto c = rayWorld.origin.dot(DprimO);

            const auto t = solveQuadraticMinPositive(a, b, c);

            cadm::vec3i rgb;
            if (!t)
            {
                rgb = cadm::vec3i();
            }
            else
            {
                cadm::vec4 intersectionPoint = rayWorld.origin + rayWorld.direction * t.value();
                cadm::vec4 pWorld(intersectionPoint.x, intersectionPoint.y, intersectionPoint.z, 1.0);
                cadm::vec4 pObject = state.Minv * pWorld;
                cadm::vec4 nObject(
                    2.0 * pObject.x / (state.a * state.a),
                    2.0 * pObject.y / (state.b * state.b),
                    2.0 * pObject.z / (state.c * state.c),
                    0.0
                );
                cadm::vec4 nWorld4 = state.MinvT * nObject;
                cadm::vec3 n(nWorld4.x, nWorld4.y, nWorld4.z);
                n.normalize();

                cadm::vec4 viewDir4 = -rayWorld.direction;
                cadm::vec3 viewDir(viewDir4.x, viewDir4.y, viewDir4.z);
                viewDir.normalize();

                const cadm::cadf cos = std::max(static_cast<cadm::cadf>(0.0), viewDir.dot(n));
                const cadm::cadf intensity = std::pow(cos, state.m);
                const cadm::vec3 specular = 255.0 * intensity * state.specularColor;

                rgb.r = static_cast<unsigned char>(std::clamp<cadm::cadf>(
                    static_cast<cadm::cadf>(state.ambient.r) + specular.x, 0.0, 255.0));
                rgb.g = static_cast<unsigned char>(std::clamp<cadm::cadf>(
                    static_cast<cadm::cadf>(state.ambient.g) + specular.y, 0.0, 255.0));
                rgb.b = static_cast<unsigned char>(std::clamp<cadm::cadf>(
                    static_cast<cadm::cadf>(state.ambient.b) + specular.z, 0.0, 255.0));
            }

            for (int dy = 0; dy < step && py + dy < h; ++dy)
            {
                for (int dx = 0; dx < step && px + dx < w; ++dx)
                {
                    const int i = ((py + dy) * w + (px + dx)) * 3;
                    buffer[i + 0] = rgb.r;
                    buffer[i + 1] = rgb.g;
                    buffer[i + 2] = rgb.b;
                }
            }
        }
    }
}

std::optional<cadm::cadf> OpenGLWidget::solveQuadraticMinPositive(const cadm::cadf a, const cadm::cadf b,
                                                                  const cadm::cadf c)
{
    // TODO: make better numerically
    if (std::abs(a) < cadm::eps)
    {
        if (std::abs(b) < cadm::eps)
        {
            return std::nullopt;
        }
        const auto result = -c / b;
        return result >= 0 ? std::optional(result) : std::nullopt;
    }

    const cadm::cadf discriminant = b * b - 4 * a * c;
    if (discriminant < 0)
    {
        return std::nullopt;
    }

    if (discriminant == 0)
    {
        if (const auto result = -b / (2 * a); result >= 0)
            return result;
        return std::nullopt;
    }

    const auto sqrt_discriminant = std::sqrt(discriminant);
    const auto result1 = (-b + sqrt_discriminant) / (2 * a);
    const auto result2 = (-b - sqrt_discriminant) / (2 * a);

    // We want the smallest positive t
    if (result1 >= 0 && result2 >= 0)
        return std::min(result1, result2);
    if (result1 >= 0)
        return result1;
    if (result2 >= 0)
        return result2;

    return std::nullopt;
}

void OpenGLWidget::mousePressEvent(QMouseEvent* event)
{
    m_lastMousePosition = event->pos();
}

void OpenGLWidget::mouseMoveEvent(QMouseEvent* event)
{
    // TODO: add time delta?
    const auto currentPos = event->pos();
    const auto delta = currentPos - m_lastMousePosition;
    m_lastMousePosition = currentPos;
    m_rotation.y += delta.x() * m_sensitivity;
    m_rotation.x += delta.y() * m_sensitivity;

    updateRenderParams();
}

void OpenGLWidget::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_W:
    case Qt::UpArrow:
        m_translation.y += m_translationStep;
        break;
    case Qt::Key_S:
    case Qt::DownArrow:
        m_translation.y -= m_translationStep;
        break;
    case Qt::Key_A:
    case Qt::LeftArrow:
        m_translation.x -= m_translationStep;
        break;
    case Qt::Key_D:
    case Qt::RightArrow:
        m_translation.x += m_translationStep;
        break;
    case Qt::Key_Q:
        m_translation.z -= m_translationStep;
        break;
    case Qt::Key_E:
        m_translation.z += m_translationStep;
        break;
    default:
        QOpenGLWidget::keyPressEvent(event);
        return;
    }
    updateRenderParams();
}

bool OpenGLWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        const auto keyEvent = dynamic_cast<QKeyEvent*>(event);
        keyPressEvent(keyEvent);
        return true;
    }
    return QObject::eventFilter(obj, event);
}
