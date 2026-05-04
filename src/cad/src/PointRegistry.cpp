//
// Created on 3/30/26.
//

#include "PointRegistry.hpp"

#include <ranges>

#include "CheckMacros.hpp"
#include "GlCommon.hpp"

PointRegistry::~PointRegistry()
{
    if (m_VAO == 0)
        return;
    const auto gl = GL();
    const GLuint buffers[3] = {m_positionVBO, m_selectedVBO, m_EBO};
    gl->glDeleteBuffers(3, buffers);
    gl->glDeleteVertexArrays(1, &m_VAO);
}

PointHandle PointRegistry::addPoint(const cadm::vec3 position)
{
    PointHandle handle;

    if (!m_freeList.empty())
    {
        handle = m_freeList.back();
        m_freeList.pop_back();
        m_positions[handle] = position;
        m_selected[handle] = 0.0f;
        m_alive[handle] = true;
    }
    else
    {
        handle = static_cast<PointHandle>(m_positions.size());
        m_positions.push_back(position);
        m_selected.push_back(0.0f);
        m_alive.push_back(true);
    }

    m_aliveHandles.push_back(handle);
    m_dirtyPositions.insert(handle);
    m_dirtySelected.insert(handle);
    m_structuralDirty = true;
    return handle;
}

void PointRegistry::removePoint(const PointHandle handle)
{
    if (handle >= m_alive.size() || !m_alive[handle])
        return;

    m_alive[handle] = false;
    m_freeList.push_back(handle);

    if (const auto it = std::ranges::find(m_aliveHandles, handle); it != m_aliveHandles.end())
    {
        // pop and replace
        *it = m_aliveHandles.back();
        m_aliveHandles.pop_back();
    }

    m_dirtyPositions.erase(handle);
    m_dirtySelected.erase(handle);
    m_structuralDirty = true;

    // snapshot before iterating: callbacks may unsubscribe during dispatch
    for (const auto callbacks = m_removeCallbacks;
         const auto &cb : callbacks | std::views::values)
    {
        cb(handle);
    }
}

void PointRegistry::setPosition(const PointHandle handle, const cadm::vec3 position)
{
    if (handle >= m_alive.size() || !m_alive[handle])
        return;
    m_positions[handle] = position;
    m_dirtyPositions.insert(handle);
    for (auto &cb : m_positionCallbacks | std::views::values)
    {
        cb(handle);
    }
}

int PointRegistry::subscribeToPositionChanges(PositionChangedCallback cb)
{
    const auto id = m_nextSubId++;
    m_positionCallbacks[id] = std::move(cb);
    return id;
}

void PointRegistry::unsubscribeFromPositionChanges(const CallbackId id)
{
    m_positionCallbacks.erase(id);
}

int PointRegistry::subscribeToRemove(RemoveCallback cb)
{
    const auto id = m_nextSubId++;
    m_removeCallbacks[id] = std::move(cb);
    return id;
}

void PointRegistry::unsubscribeFromRemove(const CallbackId id)
{
    m_removeCallbacks.erase(id);
}

cadm::vec3 PointRegistry::getPosition(const PointHandle handle) const
{
    return m_positions[handle];
}

bool PointRegistry::isAlive(const PointHandle handle) const
{
    return handle < m_alive.size() && m_alive[handle];
}

bool PointRegistry::isSelected(const PointHandle handle) const
{
    return handle < m_selected.size() && m_selected[handle] > 0.5f;
}

void PointRegistry::setSelected(const PointHandle handle, const bool selected)
{
    if (handle >= m_alive.size() || !m_alive[handle])
        return;
    const float value = selected
                            ? 1.0f
                            : 0.0f;
    if (std::abs(m_selected[handle] - value) <= cadm::feps)
        return;
    m_selected[handle] = value;
    m_dirtySelected.insert(handle);
}

void PointRegistry::clearSelection()
{
    for (const PointHandle h : m_aliveHandles)
    {
        if (m_selected[h] > 0.5f)
        {
            m_selected[h] = 0.0f;
            m_dirtySelected.insert(h);
        }
    }
}

void PointRegistry::reallocatePositionVBO(QOpenGLFunctions_4_5_Core *const gl, const GLsizeiptr posBytes) const
{
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_positionVBO);
    gl->glBufferData(GL_ARRAY_BUFFER, posBytes, nullptr, GL_DYNAMIC_DRAW);
    if (!m_positions.empty())
    {
        gl->glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            static_cast<GLsizeiptr>(m_positions.size() * 3 * GL_CADM_VT_SIZE),
            m_positions.data());
    }
}

void PointRegistry::reallocateSelectionVBO(QOpenGLFunctions_4_5_Core *const gl, const GLsizeiptr selBytes) const
{
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_selectedVBO);
    gl->glBufferData(GL_ARRAY_BUFFER, selBytes, nullptr, GL_DYNAMIC_DRAW);
    if (!m_selected.empty())
    {
        gl->glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            static_cast<GLsizeiptr>(m_selected.size() * sizeof(float)),
            m_selected.data());
    }
}

void PointRegistry::generateBuffers(QOpenGLFunctions_4_5_Core *const gl)
{
    gl->glGenVertexArrays(1, &m_VAO);
    GLuint buffers[3];
    gl->glGenBuffers(3, buffers);
    m_positionVBO = buffers[0];
    m_selectedVBO = buffers[1];
    m_EBO = buffers[2];
}

void PointRegistry::ensureGpuCapacity(const size_t requiredSlots)
{
    if (requiredSlots <= m_gpuCapacity)
        return;

    size_t newCapacity = m_gpuCapacity == 0
                             ? s_initialCapacity
                             : m_gpuCapacity * s_growFactor;
    while (newCapacity < requiredSlots)
        newCapacity *= s_growFactor;

    const auto gl = GL();
    if (m_VAO == 0)
    {
        generateBuffers(gl);
    }

    const auto posBytes = static_cast<GLsizeiptr>(newCapacity * 3 * GL_CADM_VT_SIZE);
    const auto selBytes = static_cast<GLsizeiptr>(newCapacity * sizeof(float));

    reallocatePositionVBO(gl, posBytes);
    reallocateSelectionVBO(gl, selBytes);

    // Rebind VAO attribute pointers to the new VBOs
    gl->glBindVertexArray(m_VAO);

    gl->glBindBuffer(GL_ARRAY_BUFFER, m_positionVBO);
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, GL_CADM_VT_TYPE, GL_FALSE, 3 * GL_CADM_VT_SIZE, nullptr);

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

void PointRegistry::rebuildEBO() const
{
    const auto gl = GL();
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    gl->glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_aliveHandles.size() * sizeof(PointHandle)),
        m_aliveHandles.data(),
        GL_DYNAMIC_DRAW);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    GET_GL_ERRORS();
}

void PointRegistry::flushDirtyPositions(QOpenGLFunctions_4_5_Core *const gl)
{
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_positionVBO);
    for (const PointHandle h : m_dirtyPositions)
    {
        gl->glBufferSubData(
            GL_ARRAY_BUFFER,
            static_cast<GLintptr>(h) * 3 * GL_CADM_VT_SIZE,
            3 * GL_CADM_VT_SIZE,
            &m_positions[h]);
    }
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_dirtyPositions.clear();
}

void PointRegistry::flushDirtySelection(QOpenGLFunctions_4_5_Core *const gl)
{
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_selectedVBO);
    for (const PointHandle h : m_dirtySelected)
    {
        gl->glBufferSubData(
            GL_ARRAY_BUFFER,
            static_cast<GLintptr>(static_cast<GLintptr>(h) * sizeof(float)),
            sizeof(float),
            &m_selected[h]);
    }
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_dirtySelected.clear();
}

void PointRegistry::syncToGpu()
{
    if (m_aliveHandles.empty() && m_gpuCapacity == 0)
        return;

    ensureGpuCapacity(m_positions.size());

    const auto gl = GL();
    if (!m_dirtyPositions.empty())
    {
        flushDirtyPositions(gl);
    }
    if (!m_dirtySelected.empty())
    {
        flushDirtySelection(gl);
    }

    if (m_structuralDirty)
    {
        rebuildEBO();
        m_structuralDirty = false;
    }
}
