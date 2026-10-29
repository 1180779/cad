/*
 * Created by Radosław Głasek on 28.10.2026
 * radoslaw.glasek@theworldlooksback.dev
 */

#ifndef CAD_HEADER_SRC_CNC_INCLUDE_CNC_WORKER_HXX
#define CAD_HEADER_SRC_CNC_INCLUDE_CNC_WORKER_HXX

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "cnc/Simulator.hxx"

namespace cnc
{
    /// @brief Owns the simulation timeline on a worker thread.
/// Exposes snapshots & queued cutter moves
    class Worker final
    {
    public:
        static constexpr auto gc_playbackAdvanceDeltaTime = std::chrono::milliseconds(5);

        using Command = std::function<void(Simulator &)>;
        using CommandQueue = std::deque<Command>;

        explicit Worker(Material material = {});
        ~Worker();

        Worker(const Worker&) = delete;
        Worker& operator=(const Worker&) = delete;

        void start();
        void setMaterial(Material material);
        void appendProgram(Program program);
        void clearPrograms();
        void resetMaterial();
        void setPlaying(bool playing);
        void setSpeed(double speed);
        void finishNow();
        [[nodiscard]] std::shared_ptr<const Snapshot> latestSnapshot() const;
        [[nodiscard]] ToolPath takeCutterMoves();
        [[nodiscard]] std::size_t pendingCutterMoveCount() const;
        [[nodiscard]] double speed() const;

    private:
        void post(Command command);
        void workerLoop(std::stop_token stopToken);
        void queueCutterMoves(ToolPath moves);

        Simulator m_simulator;

        mutable std::mutex m_mutex;
        std::condition_variable m_condition;

        CommandQueue m_commands;
        ToolPath m_pendingCutterMoves;
        std::shared_ptr<const Snapshot> m_latestSnapshot;

        std::atomic<double> m_speed{1.0};
        std::atomic<bool> m_finishRequested{false};
        std::jthread m_worker;

        static constexpr double gc_fixedStepSeconds = 1.0 / 120.0;
        static constexpr double gc_maxFrameSeconds = 0.25;
    };
}

#endif // CAD_HEADER_SRC_CNC_INCLUDE_CNC_WORKER_HXX
