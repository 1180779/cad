//
// Created on 3/30/26.
//

#ifndef CAD_POINTREGISTRY_HPP
#define CAD_POINTREGISTRY_HPP

#include <functional>
#include <limits>
#include <qopenglfunctions_4_5_core.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <cad_math/vec3.hpp>
#include <GL/gl.h>

#include "Callbacks.hpp"

/// Stable index into PointRegistry's slot array. Remains valid even after other
/// points are removed. Bézier curves and surfaces store these to reference
/// shared control points.
using PointHandle = uint32_t;
static constexpr PointHandle InvalidPointHandle = std::numeric_limits<uint32_t>::max();

/// Scene-level registry of all control points.
class PointRegistry
{
public:
    ~PointRegistry();

    PointHandle addPoint(cadm::vec3 position);
    void removePoint(PointHandle handle);
    void setPosition(PointHandle handle, cadm::vec3 position);

    [[nodiscard]] cadm::vec3 getPosition(PointHandle handle) const;
    [[nodiscard]] bool isAlive(PointHandle handle) const;
    [[nodiscard]] bool isSelected(PointHandle handle) const;
    [[nodiscard]] const std::vector<PointHandle>& aliveHandles() const { return m_aliveHandles; }
    [[nodiscard]] bool empty() const { return m_aliveHandles.empty(); }

    void setSelected(PointHandle handle, bool selected);
    void clearSelection();

    using PositionChangedCallback = std::function<void(PointHandle)>;
    int subscribeToPositionChanges(PositionChangedCallback cb);
    void unsubscribeFromPositionChanges(CallbackId id);

    using RemoveCallback = std::function<void(PointHandle)>;
    int subscribeToRemove(RemoveCallback cb);
    void unsubscribeFromRemove(CallbackId id);

    void syncToGpu();
    [[nodiscard]] GLuint getVAO() const { return m_VAO; }
    [[nodiscard]] GLuint getPositionVBO() const { return m_positionVBO; }
    [[nodiscard]] GLuint getEBO() const { return m_EBO; }
    [[nodiscard]] uint32_t aliveCount() const { return static_cast<uint32_t>(m_aliveHandles.size()); }

private:
    void reallocatePositionVBO(QOpenGLFunctions_4_5_Core *gl, GLsizeiptr posBytes) const;
    void reallocateSelectionVBO(QOpenGLFunctions_4_5_Core *gl, GLsizeiptr selBytes) const;
    void generateBuffers(QOpenGLFunctions_4_5_Core *gl);

    void ensureGpuCapacity(size_t requiredSlots);
    void rebuildEBO() const;
    void flushDirtyPositions(QOpenGLFunctions_4_5_Core *gl);
    void flushDirtySelection(QOpenGLFunctions_4_5_Core *gl);

    /// position for each point (handle/slot)
    std::vector<cadm::vec3> m_positions;

    /// selection state for each point (handle/slot); 0.0f or 1.0f
    std::vector<float> m_selected;

    /// alive status for each point (handle/slots) (whether the slot is taken or not)
    std::vector<bool> m_alive;

    /// list of free handles (slots)
    std::vector<PointHandle> m_freeList;
    std::vector<PointHandle> m_aliveHandles;

    std::unordered_set<PointHandle> m_dirtyPositions;
    std::unordered_set<PointHandle> m_dirtySelected;
    bool m_structuralDirty = false;

    CallbackId m_nextSubId = 0;
    std::unordered_map<CallbackId, PositionChangedCallback> m_positionCallbacks;
    std::unordered_map<CallbackId, RemoveCallback> m_removeCallbacks;

    GLuint m_VAO = 0;
    GLuint m_positionVBO = 0;
    GLuint m_selectedVBO = 0;
    GLuint m_EBO = 0;
    size_t m_gpuCapacity = 0;

    static constexpr size_t s_initialCapacity = 64;
    static constexpr size_t s_growFactor = 2.0;
};

#endif //CAD_POINTREGISTRY_HPP
