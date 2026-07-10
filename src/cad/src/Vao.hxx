//
// Created by Radosław Głasek on 24.06.2026
//

#ifndef CAD_VAO_HXX
#define CAD_VAO_HXX

#include <cassert>
#include <QOpenGLFunctions_4_5_Core>
#include <GL/gl.h>

#include "GlCommon.hpp"

/// @brief Owns a VAO configured for the project's single vec3 position attribute (location 0).
///
/// Mirrors GpuBuffer's ownership contract: no GL work in the destructor (a context cannot be
/// guaranteed at arbitrary destruction time), so owners must call deleteGpu() in their own
/// GL-context-aware destructor. The destructor only asserts the handle was released
class Vao final {
public:
    Vao() = default;

    /// @brief Create the VAO binding vbo (vec3 @ location 0) and ebo as the element buffer.
    void setup(QOpenGLFunctions_4_5_Core *gl, const GLuint vbo, const GLuint ebo) {
        assert(vbo != 0);
        assert(ebo != 0);

        gl->glGenVertexArrays(1, &m_vao);
        gl->glBindVertexArray(m_vao);
        gl->glBindBuffer(GL_ARRAY_BUFFER, vbo);
        gl->glEnableVertexAttribArray(0);
        gl->glVertexAttribPointer(0, 3, gc_glCadmVtType, GL_FALSE, 3 * gc_glCadmVtSize, nullptr);
        gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        gl->glBindVertexArray(0);
        gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
        gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void deleteGpu(QOpenGLFunctions_4_5_Core *gl) {
        if (m_vao != 0) {
            gl->glDeleteVertexArrays(1, &m_vao);
            m_vao = 0;
        }
    }

    [[nodiscard]] GLuint id() const {
        return m_vao;
    }

    [[nodiscard]] bool created() const {
        return m_vao != 0;
    }

    ~Vao() {
        assert(m_vao == 0 && "Vao destroyed with live handle; call deleteGpu() first");
    }

private:
    GLuint m_vao = 0;
};

#endif //CAD_VAO_HXX
