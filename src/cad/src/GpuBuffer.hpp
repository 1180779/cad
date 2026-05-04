//
// Created on 4/23/26.
//

#ifndef CAD_GPUBUFFER_HPP
#define CAD_GPUBUFFER_HPP

#include <algorithm>
#include <cassert>
#include <unordered_set>
#include <vector>
#include <QOpenGLFunctions_4_5_Core>
#include <GL/gl.h>

/// Generic GPU-backed buffer with capacity-growing and dirty-slot tracking.
/// Structural changes (assign/clear) trigger glBufferData; set() and in-capacity
/// append() only trigger glBufferSubData for the changed slots.
///
/// GpuBuffer has no destructor because that would require an active GL context,
/// which cannot be guaranteed at arbitrary destruction time.
/// Owners must call deleteGpu() explicitly in their own GL-context-aware destructor.
template <typename T, GLenum Target = GL_ARRAY_BUFFER>
class GpuBuffer final
{
public:
    void append(T item);
    void set(int idx, T item);
    void assign(std::vector<T> data);
    void clear();

    [[nodiscard]] int size() const { return static_cast<int>(m_data.size()); }
    [[nodiscard]] bool empty() const { return m_data.empty(); }
    T& operator[](int i) { return m_data[i]; }
    const T& operator[](int i) const { return m_data[i]; }
    const std::vector<T>& data() const { return m_data; }

    [[nodiscard]] GLuint vboId() const { return m_vbo; }

    /// Sync the cpu buffer to the gpu if needed
    void syncToGpu(QOpenGLFunctions_4_5_Core *gl, GLenum usage = GL_DYNAMIC_DRAW);

    /// Clear the gpu buffer
    /// Does not clear the cpu buffer
    void deleteGpu(QOpenGLFunctions_4_5_Core *gl);

    /// Destructor with the assertion that resources are not leaked. Does not free the gpu buffers.
    ~GpuBuffer();

private:
    /// Reallocate the Gpu buffer
    void reallocateGpu(QOpenGLFunctions_4_5_Core *gl, GLenum usage, int n);

    /// Update the gpu buffer contents with partial updates based on the dirty slots
    void partialGpuSync(QOpenGLFunctions_4_5_Core *gl);

    std::vector<T> m_data;
    std::unordered_set<int> m_dirtySlots;
    bool m_structuralDirty = true;
    int m_capacity = 0;
    GLuint m_vbo = 0;

    static constexpr int s_initialCapacity = 16;
    static constexpr int s_growFactor = 2;
};

template <typename T, GLenum Target>
void GpuBuffer<T, Target>::append(T item)
{
    m_data.push_back(std::move(item));
    if (const auto newIdx = static_cast<int>(m_data.size()) - 1;
        newIdx < m_capacity)
    {
        m_dirtySlots.insert(newIdx);
    }
    else
    {
        m_structuralDirty = true;
    }
}

template <typename T, GLenum Target>
void GpuBuffer<T, Target>::set(int idx, T item)
{
    m_data[idx] = std::move(item);
    m_dirtySlots.insert(idx);
}

template <typename T, GLenum Target>
void GpuBuffer<T, Target>::assign(std::vector<T> data)
{
    m_data = std::move(data);
    m_structuralDirty = true;
    m_dirtySlots.clear();
}

template <typename T, GLenum Target>
void GpuBuffer<T, Target>::clear()
{
    m_data.clear();
    m_structuralDirty = true;
    m_dirtySlots.clear();
}

template <typename T, GLenum Target>
void GpuBuffer<T, Target>::reallocateGpu(QOpenGLFunctions_4_5_Core *gl, const GLenum usage, const int n)
{
    int newCap = m_capacity == 0
                     ? s_initialCapacity
                     : m_capacity;
    while (newCap < n)
    {
        newCap *= s_growFactor;
    }

    gl->glBindBuffer(Target, m_vbo);
    gl->glBufferData(Target, static_cast<GLsizeiptr>(newCap * sizeof(T)), nullptr, usage);
    if (n > 0)
    {
        gl->glBufferSubData(Target, 0, static_cast<GLsizeiptr>(n * sizeof(T)), m_data.data());
    }
    gl->glBindBuffer(Target, 0);

    m_capacity = newCap;
    m_structuralDirty = false;
    m_dirtySlots.clear();
}

template <typename T, GLenum Target>
void GpuBuffer<T, Target>::partialGpuSync(QOpenGLFunctions_4_5_Core *gl)
{
    // group dirty slots that are next to each other
    std::vector sorted(m_dirtySlots.begin(), m_dirtySlots.end());
    std::ranges::sort(sorted);

    gl->glBindBuffer(Target, m_vbo);

    int blockStart = sorted[0];
    int blockEnd = sorted[0]; // inclusive
    for (int k = 1; k < static_cast<int>(sorted.size()); ++k)
    {
        if (sorted[k] == blockEnd + 1)
        {
            ++blockEnd;
        }
        else
        {
            const auto runLen = blockEnd - blockStart + 1;
            gl->glBufferSubData(
                Target,
                static_cast<GLintptr>(blockStart) * static_cast<GLintptr>(sizeof(T)),
                static_cast<GLsizeiptr>(runLen) * sizeof(T),
                &m_data[blockStart]);
            blockStart = blockEnd = sorted[k];
        }
    }

    // flush last run
    const auto runLen = blockEnd - blockStart + 1;
    gl->glBufferSubData(
        Target,
        static_cast<GLintptr>(blockStart) * static_cast<GLintptr>(sizeof(T)),
        static_cast<GLsizeiptr>(runLen) * sizeof(T),
        &m_data[blockStart]);

    gl->glBindBuffer(Target, 0);
    m_dirtySlots.clear();
}

template <typename T, GLenum Target>
void GpuBuffer<T, Target>::syncToGpu(QOpenGLFunctions_4_5_Core *gl, const GLenum usage)
{
    if (m_vbo == 0)
    {
        gl->glGenBuffers(1, &m_vbo);
        m_structuralDirty = true;
    }

    if (const auto n = static_cast<int>(m_data.size());
        m_structuralDirty || n > m_capacity)
    {
        reallocateGpu(gl, usage, n);
    }
    else if (!m_dirtySlots.empty())
    {
        partialGpuSync(gl);
    }
}

template <typename T, GLenum Target>
void GpuBuffer<T, Target>::deleteGpu(QOpenGLFunctions_4_5_Core *gl)
{
    if (m_vbo != 0)
    {
        gl->glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
        m_capacity = 0;
    }
}

template <typename T, GLenum Target>
GpuBuffer<T, Target>::~GpuBuffer()
{
    assert(m_vbo == 0 && "GpuBuffer destroyed with live VBO; resources leaked; call deleteGpu() first");
}

#endif //CAD_GPUBUFFER_HPP
