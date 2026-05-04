//
// Created on 4/18/26.
//

#include "BezierC0Component.hpp"
#include "GlCommon.hpp"

BezierC0Component::BezierC0Component(PointRegistry *registry)
    : m_registry(registry)
{
}

BezierC0Component::~BezierC0Component()
{
    const auto gl = GL();
    m_patchIndexBuf.deleteGpu(gl);
    if (m_patchVAO != 0)
    {
        gl->glDeleteVertexArrays(1, &m_patchVAO);
    }

    m_polygonLineBuf.deleteGpu(gl);
    if (m_polygonVAO != 0)
    {
        gl->glDeleteVertexArrays(1, &m_polygonVAO);
    }

    for (const auto subId : m_removeControlPointCallbacks | std::views::values)
    {
        m_registry->unsubscribeFromRemove(subId);
    }
    m_removeControlPointCallbacks.clear();
}

void BezierC0Component::addControlPoint(const PointHandle h)
{
    const auto subId = m_registry->subscribeToRemove(
        [&](auto sh)
        {
            removeControlPoint(sh);
        });
    m_removeControlPointCallbacks[h] = subId;
    m_controlPoints.push_back(h);
    markStructuralDirty();
}

void BezierC0Component::removeAssociatedCallback(const std::vector<unsigned>::value_type h)
{
    const auto subIdIter = m_removeControlPointCallbacks.find(h);
    if (subIdIter == m_removeControlPointCallbacks.end())
        return;

    const auto subId = subIdIter->first;
    m_registry->unsubscribeFromRemove(subId);
    m_removeControlPointCallbacks.erase(h);
}

void BezierC0Component::removeControlPointAt(const int index)
{
    if (index < 0 || index >= static_cast<int>(m_controlPoints.size())) return;

    const auto h = m_controlPoints[index];
    m_controlPoints.erase(m_controlPoints.begin() + index);
    removeAssociatedCallback(h);
    markStructuralDirty();
}

void BezierC0Component::removeControlPoint(const PointHandle h)
{
    if (const auto it = std::ranges::find(m_controlPoints, h);
        it != m_controlPoints.end())
    {
        m_controlPoints.erase(it);
        removeAssociatedCallback(h);
        markStructuralDirty();
    }
}

void BezierC0Component::setShowPolygon(const bool v)
{
    m_showPolygon = v;
    m_needsUpdate = true;
}

int BezierC0Component::segmentCount() const
{
    const int n = static_cast<int>(m_controlPoints.size());
    return n >= 4
               ? (n - 1) / 3
               : 0;
}

int BezierC0Component::trailingEdges() const
{
    const int n = static_cast<int>(m_controlPoints.size());
    if (n < 2) return 0;
    return (n - 1) % 3;
}

void BezierC0Component::markStructuralDirty()
{
    m_needsUpdate = true;
    m_structuralDirty = true;
}

void BezierC0Component::rebuildPatchIndices()
{
    const int segments = segmentCount();
    const int trailing = trailingEdges();
    const int totalPatches = segments + (trailing > 0
                                             ? 1
                                             : 0);

    std::vector<uint32_t> indices;
    indices.reserve(static_cast<size_t>(totalPatches) * 4);

    for (int p = 0; p < segments; ++p)
    {
        const int base = p * 3;
        for (int i = 0; i < 4; ++i)
        {
            indices.push_back(static_cast<uint32_t>(m_controlPoints[base + i]));
        }
    }

    if (trailing > 0)
    {
        const int base = segments * 3;
        const int count = trailing + 1;
        for (int i = 0; i < count; ++i)
        {
            indices.push_back(static_cast<uint32_t>(m_controlPoints[base + i]));
        }
        // pad to 4 with the last point
        const auto last = m_controlPoints[base + count - 1];
        for (int i = count; i < 4; ++i)
        {
            indices.push_back(last);
        }
    }

    m_patchIndexBuf.assign(std::move(indices));
}

void BezierC0Component::rebuildPolygonLines()
{
    const int n = static_cast<int>(m_controlPoints.size());
    std::vector<uint32_t> indices;
    indices.reserve(static_cast<size_t>(std::max(0, n - 1)) * 2);
    for (int i = 0; i + 1 < n; ++i)
    {
        indices.push_back(static_cast<uint32_t>(m_controlPoints[i]));
        indices.push_back(static_cast<uint32_t>(m_controlPoints[i + 1]));
    }
    m_polygonLineBuf.assign(std::move(indices));
}

void BezierC0Component::setupPatchVao(QOpenGLFunctions_4_5_Core *const gl)
{
    gl->glGenVertexArrays(1, &m_patchVAO);
    gl->glBindVertexArray(m_patchVAO);
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_registry->getPositionVBO());
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, GL_CADM_VT_TYPE, GL_FALSE, 3 * GL_CADM_VT_SIZE, nullptr);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_patchIndexBuf.vboId());
    gl->glBindVertexArray(0);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void BezierC0Component::setupPolygonVao(QOpenGLFunctions_4_5_Core *const gl)
{
    gl->glGenVertexArrays(1, &m_polygonVAO);
    gl->glBindVertexArray(m_polygonVAO);
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_registry->getPositionVBO());
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, GL_CADM_VT_TYPE, GL_FALSE, 3 * GL_CADM_VT_SIZE, nullptr);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_polygonLineBuf.vboId());
    gl->glBindVertexArray(0);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void BezierC0Component::regenerateMesh()
{
    if (m_structuralDirty)
    {
        rebuildPatchIndices();
        rebuildPolygonLines();
    }
}

void BezierC0Component::syncToGpu()
{
    const auto gl = GL();

    // Both EBOs must sync outside any bound VAO to avoid clobbering VAO state
    m_patchIndexBuf.syncToGpu(gl);
    m_polygonLineBuf.syncToGpu(gl);

    if (m_structuralDirty)
    {
        if (m_patchVAO != 0)
        {
            gl->glDeleteVertexArrays(1, &m_patchVAO);
            m_patchVAO = 0;
        }
        if (m_polygonVAO != 0)
        {
            gl->glDeleteVertexArrays(1, &m_polygonVAO);
            m_polygonVAO = 0;
        }
        setupPatchVao(gl);
        setupPolygonVao(gl);

        m_structuralDirty = false;
    }
}
