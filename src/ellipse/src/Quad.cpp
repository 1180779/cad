//
// Created by rdkgsk on 3/2/26.
//

#include "Quad.hpp"
#include <array>

Quad::Quad() : m_vao(0), m_vbo(0) {
    constexpr std::array<GLfloat, 20> vertices = {
        // positions        // texture Coords
        -1.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        -1.0f,
        -1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        0.0f,
        1.0f,
        1.0f,
        1.0f,
        -1.0f,
        0.0f,
        1.0f,
        0.0f
    };

    const auto gl = gl();
    gl->glGenVertexArrays(1, &m_vao);
    gl->glGenBuffers(1, &m_vbo);

    gl->glBindVertexArray(m_vao);
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    gl->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    gl->glEnableVertexAttribArray(1);
    gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));

    gl->glBindVertexArray(0);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Quad::~Quad() {
    const auto gl = gl();
    gl->glDeleteVertexArrays(1, &m_vao);
    gl->glDeleteBuffers(1, &m_vbo);
}

void Quad::draw() const {
    const auto gl = gl();
    gl->glBindVertexArray(m_vao);
    gl->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    gl->glBindVertexArray(0);
}