//
// Created on 4/23/26.
//

#ifndef CAD_GPUBUFFER_HPP
#define CAD_GPUBUFFER_HPP

#include <algorithm>
#include <cassert>
#include <ranges>
#include <span>
#include <unordered_set>
#include <vector>
#include <QOpenGLFunctions_4_5_Core>
#include <GL/gl.h>

/// @brief Generic GPU-backed buffer with capacity-growing and dirty-slot tracking.
/// Structural changes (assign/clear) trigger glBufferData; set() and in-capacity
/// append() only trigger glBufferSubData for the changed slots.
///
/// GpuBuffer has no destructor because that would require an active GL context,
/// which cannot be guaranteed at arbitrary destruction time.
/// Owners must call deleteGpu() explicitly in their own GL-context-aware destructor
template <typename T, GLenum Target = GL_ARRAY_BUFFER, GLenum DefaultUsage = GL_DYNAMIC_DRAW>
class GpuBuffer final {
public:
    /// Append one element
    /// @note marks dirty (or structurally dirty if over capacity)
    void append(T item);

    /// @brief Append a contiguous block of elements
    /// @note marks the new slots dirty, or structurally dirty when growing past
    /// capacity
    void appendRange(std::span<const T> items);

    /// Overwrite an element at idx
    /// @note marks slot dirty
    void set(int idx, T item);

    /// Replace the entire buffer
    /// @note triggers full GPU realloc on next sync
    void assign(std::vector<T> data);

    /// Empty the buffer
    /// @note triggers full GPU realloc on next sync
    void clear();

    /// Remove the last element
    /// @note no GPU work; draw count comes from size()
    void popBack();

    /// Remove the element at idx, shifting the tail left
    /// @note marks shifted slots dirty
    void eraseAt(int idx);

    /// Remove the element at idx by swapping the last element into its place.
    /// @note order is NOT preserved; touches at most one slot (cheaper than eraseAt).
    /// Use when the buffer is an unordered set (e.g., a gather/index buffer)
    void swapPopAt(int idx);

    /// Replace contents by diffing against existing data
    /// @note finds the first differing slot and marks everything from there to the end of the new data dirty.
    /// Falls back to assign() only when the new size exceeds current capacity
    void diffAssign(std::vector<T> data);

    [[nodiscard]] int size() const {
        return static_cast<int>(m_data.size());
    }

    [[nodiscard]] bool empty() const {
        return m_data.empty();
    }

    T& operator[](int i) {
        return m_data[i];
    }

    const T& operator[](int i) const {
        return m_data[i];
    }

    const std::vector<T>& data() const {
        return m_data;
    }

    [[nodiscard]] GLuint vboId() const {
        return m_vbo;
    }

    /// Upload pending changes to the GPU:
    /// reallocates if structural dirty, otherwise partial sub-data update
    void syncToGpu(QOpenGLFunctions_4_5_Core *gl, GLenum usage = DefaultUsage);

    /// Delete the GPU buffer object
    /// @note does not touch CPU data. Must be called before destruction
    void deleteGpu(QOpenGLFunctions_4_5_Core *gl);

    /// Destructor with the assertion that resources are not leaked. Does not free the gpu buffers
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

template <typename T, GLenum Target, GLenum DefaultUsage>
void GpuBuffer<T, Target, DefaultUsage>::append(T item) {
    m_data.push_back(std::move(item));
    if (const auto newIdx = static_cast<int>(m_data.size()) - 1;
        newIdx < m_capacity) {
        m_dirtySlots.insert(newIdx);
    }
    else {
        m_structuralDirty = true;
    }
}

template <typename T, GLenum Target, GLenum DefaultUsage>
void GpuBuffer<T, Target, DefaultUsage>::appendRange(const std::span<const T> items) {
    const int first = static_cast<int>(m_data.size());
    m_data.insert(m_data.end(), items.begin(), items.end());
    if (static_cast<int>(m_data.size()) > m_capacity) {
        m_structuralDirty = true;
    }
    if (m_structuralDirty) {
        return;
    }
    const auto newSlots = std::views::iota(first, static_cast<int>(m_data.size()));
    m_dirtySlots.reserve(m_dirtySlots.size() + newSlots.size());
    m_dirtySlots.insert(newSlots.begin(), newSlots.end());
}

template <typename T, GLenum Target, GLenum DefaultUsage>
void GpuBuffer<T, Target, DefaultUsage>::set(int idx, T item) {
    m_data[idx] = std::move(item);
    m_dirtySlots.insert(idx);
}

template <typename T, GLenum Target, GLenum DefaultUsage>
void GpuBuffer<T, Target, DefaultUsage>::assign(std::vector<T> data) {
    m_data = std::move(data);
    m_structuralDirty = true;
    m_dirtySlots.clear();
}

template <typename T, GLenum Target, GLenum DefaultUsage>
void GpuBuffer<T, Target, DefaultUsage>::clear() {
    m_data.clear();
    m_structuralDirty = true;
    m_dirtySlots.clear();
}

template <typename T, GLenum Target, GLenum DefaultUsage>
void GpuBuffer<T, Target, DefaultUsage>::popBack() {
    if (m_data.empty()) {
        return;
    }
    const int removed = static_cast<int>(m_data.size()) - 1;
    m_dirtySlots.erase(removed);
    m_data.pop_back();
}

template <typename T, GLenum Target, GLenum DefaultUsage>
void GpuBuffer<T, Target, DefaultUsage>::eraseAt(const int idx) {
    const int newSize = static_cast<int>(m_data.size()) - 1;
    m_data.erase(m_data.begin() + idx);
    if (m_structuralDirty) {
        return;
    }

    m_dirtySlots.erase(newSize);
    for (int i = idx; i < newSize; ++i) {
        m_dirtySlots.insert(i);
    }
}

template <typename T, GLenum Target, GLenum DefaultUsage>
void GpuBuffer<T, Target, DefaultUsage>::swapPopAt(const int idx) {
    const int last = static_cast<int>(m_data.size()) - 1;
    if (idx != last) {
        m_data[idx] = m_data[last];
        if (!m_structuralDirty) {
            m_dirtySlots.insert(idx);
        }
    }
    m_dirtySlots.erase(last);
    m_data.pop_back();
}

template <typename T, GLenum Target, GLenum DefaultUsage>
void GpuBuffer<T, Target, DefaultUsage>::diffAssign(std::vector<T> data) {
    const int newSize = static_cast<int>(data.size());
    if (newSize > m_capacity) {
        assign(std::move(data));
        return;
    }

    const int oldSize = static_cast<int>(m_data.size());
    const int commonLen = std::min(oldSize, newSize);
    int firstDiff = commonLen;
    for (int i = 0; i < commonLen; ++i) {
        if (data[i] != m_data[i]) {
            firstDiff = i;
            break;
        }
    }

    m_data = std::move(data);

    // remove dirty slots that no longer exist
    for (int i = newSize; i < oldSize; ++i) {
        m_dirtySlots.erase(i);
    }

    for (int i = firstDiff; i < newSize; ++i) {
        m_dirtySlots.insert(i);
    }
}

template <typename T, GLenum Target, GLenum DefaultUsage>
void GpuBuffer<T, Target, DefaultUsage>::reallocateGpu(QOpenGLFunctions_4_5_Core *gl, const GLenum usage, const int n) {
    int newCap = m_capacity == 0
                     ? s_initialCapacity
                     : m_capacity;
    while (newCap < n) {
        newCap *= s_growFactor;
    }

    gl->glBindBuffer(Target, m_vbo);
    gl->glBufferData(Target, static_cast<GLsizeiptr>(newCap * sizeof(T)), nullptr, usage);
    if (n > 0) {
        gl->glBufferSubData(Target, 0, static_cast<GLsizeiptr>(n * sizeof(T)), m_data.data());
    }
    gl->glBindBuffer(Target, 0);

    m_capacity = newCap;
    m_structuralDirty = false;
    m_dirtySlots.clear();
}

template <typename T, GLenum Target, GLenum DefaultUsage>
void GpuBuffer<T, Target, DefaultUsage>::partialGpuSync(QOpenGLFunctions_4_5_Core *gl) {
    // group dirty slots that are next to each other
    std::vector sorted(m_dirtySlots.begin(), m_dirtySlots.end());
    std::ranges::sort(sorted);

    gl->glBindBuffer(Target, m_vbo);

    int blockStart = sorted[0];
    int blockEnd = sorted[0]; // inclusive
    for (int k = 1; k < static_cast<int>(sorted.size()); ++k) {
        if (sorted[k] == blockEnd + 1) {
            ++blockEnd;
        }
        else {
            const auto runLen = blockEnd - blockStart + 1;
            gl->glBufferSubData(
                Target,
                static_cast<GLintptr>(blockStart) * static_cast<GLintptr>(sizeof(T)),
                static_cast<GLsizeiptr>(runLen) * sizeof(T),
                &m_data[blockStart]
            );
            blockStart = blockEnd = sorted[k];
        }
    }

    // flush last run
    const auto runLen = blockEnd - blockStart + 1;
    gl->glBufferSubData(
        Target,
        static_cast<GLintptr>(blockStart) * static_cast<GLintptr>(sizeof(T)),
        static_cast<GLsizeiptr>(runLen) * sizeof(T),
        &m_data[blockStart]
    );

    gl->glBindBuffer(Target, 0);
    m_dirtySlots.clear();
}

template <typename T, GLenum Target, GLenum DefaultUsage>
void GpuBuffer<T, Target, DefaultUsage>::syncToGpu(QOpenGLFunctions_4_5_Core *gl, const GLenum usage) {
    if (m_vbo == 0) {
        gl->glGenBuffers(1, &m_vbo);
        m_structuralDirty = true;
    }

    if (const auto n = static_cast<int>(m_data.size());
        m_structuralDirty || n > m_capacity) {
        reallocateGpu(gl, usage, n);
    }
    else if (!m_dirtySlots.empty()) {
        partialGpuSync(gl);
    }
}

template <typename T, GLenum Target, GLenum DefaultUsage>
void GpuBuffer<T, Target, DefaultUsage>::deleteGpu(QOpenGLFunctions_4_5_Core *gl) {
    if (m_vbo != 0) {
        gl->glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
        m_capacity = 0;
    }
}

template <typename T, GLenum Target, GLenum DefaultUsage>
GpuBuffer<T, Target, DefaultUsage>::~GpuBuffer() {
    assert(m_vbo == 0 && "GpuBuffer destroyed with live VBO; resources leaked; call deleteGpu() first");
}

#endif //CAD_GPUBUFFER_HPP
