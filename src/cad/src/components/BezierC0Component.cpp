//
// Created on 4/18/26.
//

#include "BezierC0Component.hpp"
#include "GlCommon.hpp"

BezierC0Component::BezierC0Component(PointRegistry *registry) : m_registry(registry) {}

BezierC0Component::~BezierC0Component() {
    const auto gl = GL();
    m_patchIndexBuf.deleteGpu(gl);
    if (m_patchVao != 0) { gl->glDeleteVertexArrays(1, &m_patchVao); }

    m_polygonIndexBuf.deleteGpu(gl);
    if (m_polygonVao != 0) { gl->glDeleteVertexArrays(1, &m_polygonVao); }

    for (const auto subId : m_removeControlPointCallbacks | std::views::values) {
        m_registry->unsubscribeFromRemove(subId);
    }
    m_removeControlPointCallbacks.clear();
}

void BezierC0Component::addControlPoint(const PointHandle h) {
    const auto subId = m_registry->subscribeToRemove(
        [&](auto sh) { removeControlPoint(sh); }
    );
    m_removeControlPointCallbacks[h] = subId;

    const int n = static_cast<int>(m_controlPoints.size());
    m_controlPoints.push_back(h);
    m_needsUpdate = true;
    m_polygonIndexBuf.append(h);

    if (n == 0) { return; }
    if (n == 1) {
        // 1 to 2 points
        // trailing becomes 1, append [p0, p1, p1, p1]
        m_patchIndexBuf.append(m_controlPoints[0]);
        m_patchIndexBuf.append(h);
        m_patchIndexBuf.append(h);
        m_patchIndexBuf.append(h);
        return;
    }

    switch ((n - 1) % 3) {
    case 0: {
        // append new trailing patch.
        const auto prev = m_controlPoints[n - 1];
        const auto cur = static_cast<uint32_t>(h);
        m_patchIndexBuf.append(prev);
        m_patchIndexBuf.append(cur);
        m_patchIndexBuf.append(cur);
        m_patchIndexBuf.append(cur);
    }
    break;
    case 1: {
        // [a, b, b, b] to [a, b, new, new]
        const auto cur = static_cast<uint32_t>(h);
        m_patchIndexBuf.set(m_patchIndexBuf.size() - 2, cur);
        m_patchIndexBuf.set(m_patchIndexBuf.size() - 1, cur);
    }
    break;
    default: {
        // trailing becomes complete segment
        // [a, b, c, c] to [a, b, c, new]
        m_patchIndexBuf.set(m_patchIndexBuf.size() - 1, h);
    }
    break;
    }
}

void BezierC0Component::removeAssociatedCallback(const PointHandle h) {
    const auto subIdIter = m_removeControlPointCallbacks.find(h);
    if (subIdIter == m_removeControlPointCallbacks.end()) { return; }

    const auto subId = subIdIter->second;
    m_registry->unsubscribeFromRemove(subId);
    m_removeControlPointCallbacks.erase(h);
}

void BezierC0Component::removeLastPointIncremental() {
    const int n = static_cast<int>(m_controlPoints.size());
    m_needsUpdate = true;

    // update polygon
    if (n >= 1) { m_polygonIndexBuf.popBack(); }

    // update patch
    if (n < 2) { return; }
    switch (trailingEdges()) {
    case 1: {
        // [a, b, b, b]
        // remove trailing patch entirely
        m_patchIndexBuf.popBack();
        m_patchIndexBuf.popBack();
        m_patchIndexBuf.popBack();
        m_patchIndexBuf.popBack();
        break;
    }
    case 2: {
        // [a, b, c, c] to [a, b, b, b]
        const auto prev = m_controlPoints[n - 2];
        m_patchIndexBuf.set(m_patchIndexBuf.size() - 2, prev);
        m_patchIndexBuf.set(m_patchIndexBuf.size() - 1, prev);
        break;
    }
    default: {
        // last segment is complete
        // [a, b, c, d] to trailing [a, b, c, c]
        const auto prev = m_controlPoints[n - 2];
        m_patchIndexBuf.set(m_patchIndexBuf.size() - 1, prev);
    }
    }
}

void BezierC0Component::removeControlPointAt(const int index) {
    if (index < 0 || index >= static_cast<int>(m_controlPoints.size())) { return; }

    m_needsUpdate = true;
    const auto h = m_controlPoints[index];
    const bool isLast = index == static_cast<int>(m_controlPoints.size()) - 1;

    if (isLast) { removeLastPointIncremental(); }

    m_controlPoints.erase(m_controlPoints.begin() + index);
    removeAssociatedCallback(h);

    if (!isLast) { removeMidPointPartial(index); }
}

void BezierC0Component::removeControlPoint(const PointHandle h) {
    const auto it = std::ranges::find(m_controlPoints, h);
    if (it == m_controlPoints.end()) { return; }

    m_needsUpdate = true;
    const int index = static_cast<int>(it - m_controlPoints.begin());
    const bool isLast = it == m_controlPoints.end() - 1;

    if (isLast) { removeLastPointIncremental(); }

    m_controlPoints.erase(it);
    removeAssociatedCallback(h);

    if (!isLast) { removeMidPointPartial(index); }
}

void BezierC0Component::removeMidPointPartial(const int removedIndex) {
    m_needsUpdate = true;
    m_polygonIndexBuf.eraseAt(removedIndex);
    rebuildPatchIndices();
}

void BezierC0Component::setShowPolygon(const bool v) {
    m_showPolygon = v;
    m_needsUpdate = true;
}

int BezierC0Component::segmentCount() const {
    const int n = static_cast<int>(m_controlPoints.size());
    return n >= 4
               ? (n - 1) / 3
               : 0;
}

int BezierC0Component::trailingEdges() const {
    const int n = static_cast<int>(m_controlPoints.size());
    if (n < 2) { return 0; }
    return (n - 1) % 3;
}

void BezierC0Component::rebuildPatchIndices() {
    const int segments = segmentCount();
    const int trailing = trailingEdges();
    const int totalPatches = segments + (trailing > 0
                                             ? 1
                                             : 0);

    std::vector<uint32_t> indices;
    indices.reserve(static_cast<size_t>(totalPatches) * 4);

    for (int p = 0; p < segments; ++p) {
        const int base = p * 3;
        for (int i = 0; i < 4; ++i) { indices.push_back(static_cast<uint32_t>(m_controlPoints[base + i])); }
    }

    if (trailing > 0) {
        const int base = segments * 3;
        const int count = trailing + 1;
        for (int i = 0; i < count; ++i) { indices.push_back(static_cast<uint32_t>(m_controlPoints[base + i])); }
        // pad to 4 with the last point
        const auto last = m_controlPoints[base + count - 1];
        for (int i = count; i < 4; ++i) { indices.push_back(last); }
    }

    m_patchIndexBuf.diffAssign(std::move(indices));
}

void BezierC0Component::rebuildPolygonLines() {
    const int n = static_cast<int>(m_controlPoints.size());
    std::vector<uint32_t> indices;
    indices.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) { indices.push_back(static_cast<uint32_t>(m_controlPoints[i])); }
    m_polygonIndexBuf.assign(std::move(indices));
}

void BezierC0Component::setupPatchVao(QOpenGLFunctions_4_5_Core * const gl) {
    assert(m_registry->getPositionVBO() != 0);
    assert(m_patchIndexBuf.vboId() != 0);

    gl->glGenVertexArrays(1, &m_patchVao);
    gl->glBindVertexArray(m_patchVao);
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_registry->getPositionVBO());
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, GL_CADM_VT_TYPE, GL_FALSE, 3 * GL_CADM_VT_SIZE, nullptr);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_patchIndexBuf.vboId());
    gl->glBindVertexArray(0);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void BezierC0Component::setupPolygonVao(QOpenGLFunctions_4_5_Core * const gl) {
    assert(m_registry->getPositionVBO() != 0);
    assert(m_polygonIndexBuf.vboId() != 0);

    gl->glGenVertexArrays(1, &m_polygonVao);
    gl->glBindVertexArray(m_polygonVao);
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_registry->getPositionVBO());
    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, GL_CADM_VT_TYPE, GL_FALSE, 3 * GL_CADM_VT_SIZE, nullptr);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_polygonIndexBuf.vboId());
    gl->glBindVertexArray(0);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void BezierC0Component::regenerateMesh() {}

void BezierC0Component::syncToGpu() {
    const auto gl = GL();

    m_patchIndexBuf.syncToGpu(gl);
    m_polygonIndexBuf.syncToGpu(gl);

    if (m_patchVao == 0) { setupPatchVao(gl); }
    if (m_polygonVao == 0) { setupPolygonVao(gl); }
}
