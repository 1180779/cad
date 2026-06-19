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

#include <cad_math/Vec3.hpp>
#include <GL/gl.h>

#include "Callbacks.hpp"
#include "GpuBuffer.hpp"

/// @brief Stable index into PointRegistry's slot array. Remains valid even after other
/// points are removed. Bézier curves and surfaces store these to reference
/// shared control points
using PointHandle = uint32_t;

static constexpr PointHandle InvalidPointHandle = std::numeric_limits<uint32_t>::max();

/// Scene-level registry of all control points.
///
/// TODO: slots are never reclaimed structurally; a freed slot stays as a hole in
///     the sparse position/selection VBOs until reused, so heavy churn 
///     (create/delete cycles) leaves the arrays as large as the high-water mark. 
///     A future "relocate" pass could compact live points down, remap their handles 
///     (updating every referrer: PointComponents, Bézier control-point lists, the index buffer), 
///     and shrink the VBOs. Non-trivial because handles are currently == VBO slot index and
///     are referenced widely; out of scope for now
class PointRegistry {
public:
    ~PointRegistry();

    PointHandle addPoint(cadm::Vec3 position);

    /// @brief Resurrect a previously removed point at a specific handle so existing references 
    /// (e.g., Bézier control point lists) stay valid after undo.
    /// The slot must currently be dead (freed) or beyond the current range
    void addPointAt(PointHandle handle, cadm::Vec3 position);

    void removePoint(PointHandle handle);

    void setPosition(PointHandle handle, cadm::Vec3 position);

    [[nodiscard]] cadm::Vec3 getPosition(PointHandle handle) const;

    [[nodiscard]] bool isAlive(PointHandle handle) const;

    [[nodiscard]] bool isSelected(PointHandle handle) const;

    [[nodiscard]] const std::vector<PointHandle>& aliveHandles() const {
        return m_indexBuffer.data();
    }

    [[nodiscard]] bool empty() const {
        return m_indexBuffer.empty();
    }

    void setSelected(PointHandle handle, bool selected);

    void clearSelection();

    using PositionChangedCallback = std::function<void(PointHandle)>;

    int subscribeToPositionChanges(PositionChangedCallback cb);

    void unsubscribeFromPositionChanges(CallbackId id);

    using RemoveCallback = std::function<void(PointHandle)>;

    int subscribeToRemove(RemoveCallback cb);

    void unsubscribeFromRemove(CallbackId id);

    void initialize();

    void syncToGpu();

    [[nodiscard]] GLuint getVAO() const {
        return m_VAO;
    }

    [[nodiscard]] GLuint getPositionVBO() const {
        return m_positionVBO;
    }

    // ReSharper disable once CppDFAConstantFunctionResult

    [[nodiscard]] GLuint getEBO() const {
        return m_indexBuffer.vboId();
    }

    [[nodiscard]] uint32_t aliveCount() const {
        return static_cast<uint32_t>(m_indexBuffer.size());
    }

private:
    void reallocatePositionVBO(QOpenGLFunctions_4_5_Core *gl, GLsizeiptr posBytes) const;

    void reallocateSelectionVBO(QOpenGLFunctions_4_5_Core *gl, GLsizeiptr selBytes) const;

    void generateBuffers(QOpenGLFunctions_4_5_Core *gl);

    void ensureGpuCapacity(size_t requiredSlots);

    void flushDirtyPositions(QOpenGLFunctions_4_5_Core *gl);

    void flushDirtySelection(QOpenGLFunctions_4_5_Core *gl);

    /// @brief Position for each point (handle/slot)
    std::vector<cadm::Vec3> m_positions;

    /// @brief Selection state for each point (handle/slot); 0.0f or 1.0f
    std::vector<float> m_selected;

    /// @brief Alive status for each point (handle/slots) (whether the slot is taken or not)
    std::vector<bool> m_alive;

    /// @brief List of free handles (slots)
    std::vector<PointHandle> m_freeList;

    /// @brief Packed list of live slot indices, used as the draw index buffer (EBO).
    /// The position/selection VBOs are sparse (indexed by a handle), so this gather
    /// buffer selects only live slots at draw time
    GpuBuffer<PointHandle, GL_ELEMENT_ARRAY_BUFFER> m_indexBuffer;

    std::unordered_set<PointHandle> m_dirtyPositions;
    std::unordered_set<PointHandle> m_dirtySelected;

    CallbackId m_nextSubId = 0;
    std::unordered_map<CallbackId, PositionChangedCallback> m_positionCallbacks;
    std::unordered_map<CallbackId, RemoveCallback> m_removeCallbacks;

    GLuint m_VAO = 0;
    GLuint m_positionVBO = 0;
    GLuint m_selectedVBO = 0;
    size_t m_gpuCapacity = 0;

    static constexpr size_t s_initialCapacity = 64;
    static constexpr size_t s_growFactor = 2.0;
};

#endif //CAD_POINTREGISTRY_HPP
