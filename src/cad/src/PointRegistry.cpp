//
// Created on 3/30/26.
//

#include "PointRegistry.hpp"

#include <algorithm>
#include <numeric>
#include <ranges>

#include "CheckMacros.hpp"
#include "GlCommon.hpp"

PointRegistry::~PointRegistry() {
    if (m_VAO == 0) {
        return;
    }
    const auto gl = getGl();
    const GLuint buffers[2] = {m_positionVBO, m_selectedVBO};
    gl->glDeleteBuffers(2, buffers);
    gl->glDeleteVertexArrays(1, &m_VAO);
    m_indexBuffer.deleteGpu(gl);
}

PointHandle PointRegistry::addPoint(const cadm::Vec3 position) {
    PointHandle handle;

    if (!m_freeList.empty()) {
        handle = m_freeList.back();
        m_freeList.pop_back();
        m_positions[handle] = position;
        m_selected[handle] = 0.0f;
        m_alive[handle] = true;
    }
    else {
        handle = static_cast<PointHandle>(m_positions.size());
        m_positions.push_back(position);
        m_selected.push_back(0.0f);
        m_alive.push_back(true);
    }

    m_indexBuffer.append(handle);
    m_dirtyPositions.insert(handle);
    m_dirtySelected.insert(handle);
    return handle;
}

PointHandle PointRegistry::addPoints(const std::span<const cadm::Vec3> positions) {
    const size_t count = positions.size();
    if (count == 0) {
        return InvalidPointHandle;
    }

    // register the claimed slots [first, first + count) as alive and dirty
    const auto registerRange = [this, count](const PointHandle first) {
        std::vector<PointHandle> handles(count);
        std::ranges::iota(handles, first);
        m_indexBuffer.appendRange(handles);
        m_dirtyPositions.insert(handles.begin(), handles.end());
        m_dirtySelected.insert(handles.begin(), handles.end());
    };

    // reuse a contiguous run of freed slots before growing the arrays, so bulk
    // create/delete cycles don't ratchet the buffers up forever
    std::ranges::sort(m_freeList);
    size_t runLen = 0;
    for (size_t i = 0; i < m_freeList.size(); ++i) {
        runLen = i > 0 && m_freeList[i] == m_freeList[i - 1] + 1
                     ? runLen + 1
                     : 1;
        if (runLen == count) {
            const auto runBegin = m_freeList.begin() + static_cast<ptrdiff_t>(i + 1 - count);
            const PointHandle first = *runBegin;
            m_freeList.erase(runBegin, runBegin + static_cast<ptrdiff_t>(count));

            std::ranges::copy(positions, m_positions.begin() + first);
            std::fill_n(m_selected.begin() + first, count, 0.0f);
            std::fill_n(m_alive.begin() + first, count, true);
            registerRange(first);
            return first;
        }
    }

    // otherwise append at the end
    const auto first = static_cast<PointHandle>(m_positions.size());
    m_positions.insert(m_positions.end(), positions.begin(), positions.end());
    m_selected.insert(m_selected.end(), count, 0.0f);
    m_alive.insert(m_alive.end(), count, true);
    registerRange(first);
    return first;
}

void PointRegistry::setPositions(const PointHandle first, const std::span<const cadm::Vec3> positions) {
    if (first + positions.size() > m_positions.size()) {
        return;
    }
    std::ranges::copy(positions, m_positions.begin() + first);

    const auto handles = std::views::iota(first, first + static_cast<PointHandle>(positions.size()));
    m_dirtyPositions.reserve(m_dirtyPositions.size() + handles.size());
    m_dirtyPositions.insert(handles.begin(), handles.end());

    for (const PointHandle h : handles) {
        for (auto &cb : m_positionCallbacks | std::views::values) {
            cb(h);
        }
    }
}

void PointRegistry::addPointAt(const PointHandle handle, const cadm::Vec3 position) {
    // if the slot is already live, this is a reload/overwrite, not a fresh insert:
    // update in place and don't duplicate the handle in the index buffer
    if (handle < m_alive.size() && m_alive[handle]) {
        m_positions[handle] = position;
        m_selected[handle] = 0.0f;
        m_dirtyPositions.insert(handle);
        m_dirtySelected.insert(handle);
        return;
    }

    // grow the slot arrays up to and including handle, marking any gap slots as free
    // ReSharper disable once CppDFALoopConditionNotUpdated
    while (m_positions.size() <= handle) {
        const auto slot = static_cast<PointHandle>(m_positions.size());
        m_positions.emplace_back();
        m_selected.push_back(0.0f);
        m_alive.push_back(false);
        if (slot != handle) {
            m_freeList.push_back(slot);
        }
    }

    // reclaim the exact slot from the free list if present
    if (const auto it = std::ranges::find(m_freeList, handle);
        it != m_freeList.end()) {
        *it = m_freeList.back();
        m_freeList.pop_back();
    }

    m_positions[handle] = position;
    m_selected[handle] = 0.0f;
    m_alive[handle] = true;

    m_indexBuffer.append(handle);
    m_dirtyPositions.insert(handle);
    m_dirtySelected.insert(handle);
}

void PointRegistry::lock(const PointHandle handle) {
    ++m_lockCounts[handle];
}

void PointRegistry::unlock(const PointHandle handle) {
    const auto it = m_lockCounts.find(handle);
    if (it == m_lockCounts.end()) {
        return;
    }
    if (--it->second <= 0) {
        m_lockCounts.erase(it);
    }
}

bool PointRegistry::isLocked(const PointHandle handle) const {
    const auto it = m_lockCounts.find(handle);
    return it != m_lockCounts.end() && it->second > 0;
}

bool PointRegistry::removePoint(const PointHandle handle) {
    if (handle >= m_alive.size() || !m_alive[handle] || isLocked(handle)) {
        return false;
    }

    m_alive[handle] = false;
    m_freeList.push_back(handle);

    // swap-and-pop the handle out of the packed index buffer (order is irrelevant
    // for a gather buffer, so this touches at most one slot on the GPU)
    const auto &handles = m_indexBuffer.data();
    if (const auto it = std::ranges::find(handles, handle);
        it != handles.end()) {
        m_indexBuffer.swapPopAt(static_cast<int>(it - handles.begin()));
    }

    m_dirtyPositions.erase(handle);
    m_dirtySelected.erase(handle);

    // snapshot before iterating: callbacks may unsubscribe during dispatch
    for (const auto callbacks = m_removeCallbacks;
         const auto &cb : callbacks | std::views::values) {
        cb(handle);
    }
    return true;
}

void PointRegistry::setPosition(const PointHandle handle, const cadm::Vec3 position) {
    if (handle >= m_alive.size() || !m_alive[handle]) {
        return;
    }
    m_positions[handle] = position;
    m_dirtyPositions.insert(handle);
    for (auto &cb : m_positionCallbacks | std::views::values) {
        cb(handle);
    }
}

int PointRegistry::subscribeToPositionChanges(PositionChangedCallback cb) {
    const auto id = m_nextSubId++;
    m_positionCallbacks[id] = std::move(cb);
    return id;
}

void PointRegistry::unsubscribeFromPositionChanges(const CallbackId id) {
    m_positionCallbacks.erase(id);
}

int PointRegistry::subscribeToRemove(RemoveCallback cb) {
    const auto id = m_nextSubId++;
    m_removeCallbacks[id] = std::move(cb);
    return id;
}

void PointRegistry::unsubscribeFromRemove(const CallbackId id) {
    m_removeCallbacks.erase(id);
}

cadm::Vec3 PointRegistry::getPosition(const PointHandle handle) const {
    return m_positions[handle];
}

bool PointRegistry::isAlive(const PointHandle handle) const {
    return handle < m_alive.size() && m_alive[handle];
}

bool PointRegistry::isSelected(const PointHandle handle) const {
    return handle < m_selected.size() && m_selected[handle] > 0.5f;
}

void PointRegistry::setSelected(const PointHandle handle, const bool selected) {
    if (handle >= m_alive.size() || !m_alive[handle]) {
        return;
    }
    const float value = selected
                            ? 1.0f
                            : 0.0f;
    if (std::abs(m_selected[handle] - value) <= cadm::gc_feps) {
        return;
    }
    m_selected[handle] = value;
    m_dirtySelected.insert(handle);
}

void PointRegistry::clear() {
    const auto callbacks = m_removeCallbacks;
    for (const auto &handles = m_indexBuffer.data();
         const PointHandle h : handles) {
        for (const auto &cb : callbacks | std::views::values) {
            cb(h);
        }
    }
    m_positions.clear();
    m_selected.clear();
    m_alive.clear();
    m_freeList.clear();
    m_indexBuffer.clear();
    m_dirtyPositions.clear();
    m_dirtySelected.clear();
    m_lockCounts.clear();
    m_positionCallbacks.clear();
    m_removeCallbacks.clear();
}

void PointRegistry::clearSelection() {
    for (const PointHandle h : m_indexBuffer.data()) {
        if (m_selected[h] > 0.5f) {
            m_selected[h] = 0.0f;
            m_dirtySelected.insert(h);
        }
    }
}

void PointRegistry::reallocatePositionVBO(
    QOpenGLFunctions_4_5_Core *const gl,

    const GLsizeiptr posBytes
)
const {
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_positionVBO);
    gl->glBufferData(GL_ARRAY_BUFFER, posBytes, nullptr, GL_DYNAMIC_DRAW);
    if (!m_positions.empty()) {
        gl->glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            static_cast<GLsizeiptr>(m_positions.size() * 3 * gc_glCadmVtSize),
            m_positions.data()
        );
    }
}

void PointRegistry::reallocateSelectionVBO(
    QOpenGLFunctions_4_5_Core *const gl,

    const GLsizeiptr selBytes
)
const {
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_selectedVBO);
    gl->glBufferData(GL_ARRAY_BUFFER, selBytes, nullptr, GL_DYNAMIC_DRAW);
    if (!m_selected.empty()) {
        gl->glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            static_cast<GLsizeiptr>(m_selected.size() * sizeof(float)),
            m_selected.data()
        );
    }
}

void PointRegistry::generateBuffers(QOpenGLFunctions_4_5_Core *const gl) {
    gl->glGenVertexArrays(1, &m_VAO);
    GLuint buffers[2];
    gl->glGenBuffers(2, buffers);
    m_positionVBO = buffers[0];
    m_selectedVBO = buffers[1];
    // m_indexBuffer lazily generates its own VBO on first syncToGpu
}

void PointRegistry::ensureGpuCapacity(const size_t requiredSlots) {
    if (requiredSlots <= m_gpuCapacity) {
        return;
    }

    size_t newCapacity = m_gpuCapacity == 0
                             ? s_initialCapacity
                             : m_gpuCapacity * s_growFactor;
    while (newCapacity < requiredSlots) {
        newCapacity *= s_growFactor;
    }

    const auto gl = getGl();
    if (m_VAO == 0) {
        generateBuffers(gl);
    }

    const auto posBytes = static_cast<GLsizeiptr>(newCapacity * 3 * gc_glCadmVtSize);
    const auto selBytes = static_cast<GLsizeiptr>(newCapacity * sizeof(float));

    reallocatePositionVBO(gl, posBytes);
    reallocateSelectionVBO(gl, selBytes);

    // rebind VAO attribute pointers to the new VBOs
    gl->glBindVertexArray(m_VAO);

    gl->glBindBuffer(GL_ARRAY_BUFFER, m_positionVBO);
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, gc_glCadmVtType, GL_FALSE, 3 * gc_glCadmVtSize, nullptr);

    gl->glBindBuffer(GL_ARRAY_BUFFER, m_selectedVBO);
    gl->glEnableVertexAttribArray(1);
    gl->glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(float), nullptr);

    // gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);

    gl->glBindVertexArray(0);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    // gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    m_gpuCapacity = newCapacity;
    m_dirtyPositions.clear();
    m_dirtySelected.clear();
    GET_GL_ERRORS();
}

void PointRegistry::flushDirtyPositions(QOpenGLFunctions_4_5_Core *const gl) {
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_positionVBO);
    for (const PointHandle h : m_dirtyPositions) {
        gl->glBufferSubData(
            GL_ARRAY_BUFFER,
            static_cast<GLintptr>(h) * 3 * gc_glCadmVtSize,
            3 * gc_glCadmVtSize,
            &m_positions[h]
        );
    }
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_dirtyPositions.clear();
}

void PointRegistry::flushDirtySelection(QOpenGLFunctions_4_5_Core *const gl) {
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_selectedVBO);
    for (const PointHandle h : m_dirtySelected) {
        gl->glBufferSubData(
            GL_ARRAY_BUFFER,
            static_cast<GLintptr>(static_cast<GLintptr>(h) * sizeof(float)),
            sizeof(float),
            &m_selected[h]
        );
    }
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_dirtySelected.clear();
}

void PointRegistry::initialize() {
    const auto gl = getGl();
    if (m_VAO == 0) {
        generateBuffers(gl);
    }

    constexpr auto posBytes = static_cast<GLsizeiptr>(s_initialCapacity * 3 * gc_glCadmVtSize);
    constexpr auto selBytes = static_cast<GLsizeiptr>(s_initialCapacity * sizeof(float));
    reallocatePositionVBO(gl, posBytes);
    reallocateSelectionVBO(gl, selBytes);

    gl->glBindVertexArray(m_VAO);
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_positionVBO);
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, gc_glCadmVtType, GL_FALSE, 3 * gc_glCadmVtSize, nullptr);
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_selectedVBO);
    gl->glEnableVertexAttribArray(1);
    gl->glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(float), nullptr);
    gl->glBindVertexArray(0);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_gpuCapacity = s_initialCapacity;
}

void PointRegistry::syncToGpu() {
    if (m_indexBuffer.empty() && m_gpuCapacity == 0) {
        return;
    }

    ensureGpuCapacity(m_positions.size());

    const auto gl = getGl();
    if (!m_dirtyPositions.empty()) {
        flushDirtyPositions(gl);
    }
    if (!m_dirtySelected.empty()) {
        flushDirtySelection(gl);
    }

    m_indexBuffer.syncToGpu(gl);
}
