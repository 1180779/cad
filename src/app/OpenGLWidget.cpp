//
// Created by rdkgsk on 3/1/26.
//

#include "OpenGLWidget.h"

#include "cad_math/mat4.h"

OpenGLWidget::OpenGLWidget(QWidget* parent) : QOpenGLWidget(parent)
{
}

void OpenGLWidget::paintGL()
{
    // TODO: implement adaptation

    const auto gl = GL();
    const auto t1 = std::chrono::high_resolution_clock::now();
    fillCpuBuffer();
    const auto t2 = std::chrono::high_resolution_clock::now();
    const auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    qInfo() << "Generating cpu buffer took: " << durationMs.count() << "ms";

    gl->glBindTexture(GL_TEXTURE_2D, m_texture);
    gl->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width(), height(), GL_RGB, GL_UNSIGNED_BYTE, m_cpuBuffer.data());

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

    qInfo() << "Resized to " << width << "x" << height;
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
    update();
}

void OpenGLWidget::setB(const cadm::cadf b)
{
    if (m_b == b)
        return;
    m_b = b;
    update();
}

void OpenGLWidget::setC(const cadm::cadf c)
{
    if (m_c == c)
        return;
    m_c = c;
    update();
}

void OpenGLWidget::setTranslation(cadm::vec3 translation)
{
    if (m_translation == translation)
        return;
    m_translation = translation;
    update();
}

void OpenGLWidget::setRotation(cadm::vec3 rotation)
{
    if (m_rotation == rotation)
        return;
    m_rotation = rotation;
    update();
}

void OpenGLWidget::setScale(cadm::vec3 scale)
{
    if (m_scale == scale)
        return;
    m_scale = scale;
    update();
}

void OpenGLWidget::setAdaptationSize(const unsigned char adaptationSize)
{
    m_adaptationSize = adaptationSize;
}

void OpenGLWidget::setM(cadm::cadf m)
{
    if (m_m == m)
        return;
    m_m = m;
    update();
}

void OpenGLWidget::setAmbientR(int r)
{
    if (m_ambient.r == r)
        return;
    m_ambient.r = r;
    update();
}

void OpenGLWidget::setAmbientG(int g)
{
    if (m_ambient.g == g)
        return;
    m_ambient.g = g;
    update();
}

void OpenGLWidget::setAmbientB(int b)
{
    if (m_ambient.b == b)
        return;
    m_ambient.b = b;
    update();
}

void OpenGLWidget::fillCpuBufferTest()
{
    const int w = width();
    const int h = height();
    if (m_cpuBuffer.size() != w * h * 3)
    {
        qWarning() << "The cpu buffer size is not correct!";
        return;
    }

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const int r = (x * x + y * y) % 256;
            const int g = (x * 3 + y * 7) % 256;
            const int b = (x ^ y) & 255;

            const int i = (y * w + x) * 3;
            m_cpuBuffer[i + 0] = static_cast<unsigned char>(r);
            m_cpuBuffer[i + 1] = static_cast<unsigned char>(g);
            m_cpuBuffer[i + 2] = static_cast<unsigned char>(b);
        }
    }
}

void OpenGLWidget::fillCpuBuffer()
{
    const int w = width();
    const int h = height();

    if (m_cpuBuffer.size() != w * h * 3)
    {
        qWarning() << "The cpu buffer size is not correct!";
        return;
    }

    const auto translationM = cadm::mat4::translation(m_translation);
    const auto rotationM = cadm::mat4::rotX(m_rotation.x) * cadm::mat4::rotY(m_rotation.y) *
        cadm::mat4::rotZ(m_rotation.z);
    const auto scaleM = cadm::mat4::scale(m_scale);
    const auto M = translationM * rotationM * scaleM;

    const auto D = cadm::mat4::diag(1.0 / (m_a * m_a), 1.0 / (m_b * m_b), 1.0 / (m_c * m_c), -1.0);
    const auto Minv = M.inverse();
    const auto MinvT = Minv.transposed();
    const auto Dprim = MinvT * D * Minv;

    const cadm::cadf aspect = static_cast<cadm::cadf>(w) / static_cast<cadm::cadf>(h);

    for (int py = 0; py < h; ++py)
    {
        for (int px = 0; px < w; ++px)
        {
            const auto x = static_cast<cadm::cadf>((2.0 * px - w) / w * aspect);
            const auto y = static_cast<cadm::cadf>((h - 2.0 * py) / h);

            const auto a = Dprim.m22;
            const auto b = 2 * Dprim.m20 * x + 2 * Dprim.m12 * y + 2 * Dprim.m23;
            const auto c = Dprim.m00 * x * x + Dprim.m11 * y * y + Dprim.m33 + 2 * Dprim.m01 * x * y + 2 * Dprim.m13 * y
                + 2 * Dprim.m03 * x;

            const auto z = solveQuadratic(a, b, c);
            const int i = (py * w + px) * 3;
            if (!z)
            {
                m_cpuBuffer[i + 0] = 0;
                m_cpuBuffer[i + 1] = 0;
                m_cpuBuffer[i + 2] = 0;
            }
            else
            {
                cadm::vec4 pScreen(x, y, z.value(), 1.0);
                cadm::vec4 pObject = Minv * pScreen;
                cadm::vec4 nObject(
                    2.0 * pObject.x / (m_a * m_a),
                    2.0 * pObject.y / (m_b * m_b),
                    2.0 * pObject.z / (m_c * m_c),
                    0.0
                );
                cadm::vec4 nWorld4 = MinvT * nObject;
                cadm::vec3 n(nWorld4.x, nWorld4.y, nWorld4.z);
                n.normalize();

                const auto specular = std::pow(std::max(static_cast<cadm::cadf>(0.0), m_v.dot(n)), m_m);
                const auto color = 255 * specular * m_specularColor;

                m_cpuBuffer[i + 0] = static_cast<unsigned char>(std::clamp<cadm::cadf>(
                    static_cast<cadm::cadf>(m_ambient.x) + color.x, 0.0, 255.0));
                m_cpuBuffer[i + 1] = static_cast<unsigned char>(std::clamp<cadm::cadf>(
                    static_cast<cadm::cadf>(m_ambient.y) + color.y, 0.0, 255.0));
                m_cpuBuffer[i + 2] = static_cast<unsigned char>(std::clamp<cadm::cadf>(
                    static_cast<cadm::cadf>(m_ambient.z) + color.z, 0.0, 255.0));
            }
        }
    }
}

std::optional<cadm::cadf> OpenGLWidget::solveQuadratic(cadm::cadf a, cadm::cadf b, cadm::cadf c)
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
    if (result1 >= 0 && result2 >= 0)
    {
        return std::min(result1, result2);
    }
    if (result1 >= 0)
    {
        return result1;
    }
    if (result2 >= 0)
    {
        return result2;
    }
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

    update();
}
