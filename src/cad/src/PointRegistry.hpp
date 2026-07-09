//
// Created on 3/30/26.
//

#ifndef CAD_POINTREGISTRY_HPP
#define CAD_POINTREGISTRY_HPP

#include <functional>
#include <limits>
#include <span>
#include <qopenglfunctions_4_5_core.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <cad_math/Vec3.hpp>
#include <GL/gl.h>

#include "Callbacks.hpp"
#include "GpuBuffer.hpp"
#include "PointHandle.hxx"

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

    /// @brief Allocate positions.size() points in contiguous slots and return
    /// the first handle; slot <code>i</code> holds <code>positions[i]</code>
    /// @note Reuses a contiguous run of freed slots when one is large enough,
    /// otherwise appends at the end
    PointHandle addPoints(std::span<const cadm::Vec3> positions);

    /// @brief Overwrite the positions of the contiguous alive range starting at
    /// first (e.g., one allocated by @ref addPoints) with a single bulk copy
    /// @note Position-change callbacks still fire per handle
    /// @pre first + positions.size() <= current slot count; a violating call is
    /// a no-op
    void setPositions(PointHandle first, std::span<const cadm::Vec3> positions);

    /// @brief Resurrect a previously removed point at a specific handle so
    /// existing references (e.g., Bézier control point lists) stay valid after
    /// undo. The slot must currently be dead (freed) or beyond the current
    /// range
    void addPointAt(PointHandle handle, cadm::Vec3 position);

    /// @brief Remove a point. Returns false without effect if the handle is
    /// dead or locked
    bool removePoint(PointHandle handle);

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

    /// @brief Take a lock on a point, refusing @ref removePoint while any lock
    /// is held
    /// @note Refcounted like a shared_ptr: nested/independent owners (e.g. two
    /// patches sharing a point) each add their own lock and must each unlock
    /// before removal is allowed. The registry doesn't know or care who holds a
    /// lock or why
    void lock(PointHandle handle);

    /// @brief Release one lock taken by @ref lock
    void unlock(PointHandle handle);

    [[nodiscard]] bool isLocked(PointHandle handle) const;

    /// @brief Drop all points
    /// @note Keeps the GPU buffers/capacity allocated for reuse
    void clear();

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

    /// @brief Lock refcount per handle; absent/0 means unlocked
    std::unordered_map<PointHandle, int> m_lockCounts;

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
