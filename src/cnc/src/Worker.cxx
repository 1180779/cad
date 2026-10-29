/*
 * Created by Radosław Głasek on 28.10.2026
 * radoslaw.glasek@theworldlooksback.dev
 */

#include "cnc/Worker.hxx"

#include <algorithm>
#include <chrono>
#include <iterator>
#include <utility>

namespace cnc
{
    Worker::Worker(Material material)
        : m_simulator(material)
    {
    }

    Worker::~Worker()
    {
        if (m_worker.joinable())
        {
            {
                std::lock_guard lock(m_mutex);
                m_worker.request_stop();
            }
            m_condition.notify_all();
            m_worker.join();
        }
    }

    void Worker::start()
    {
        if (m_worker.joinable())
        {
            return;
        }
        m_worker = std::jthread([this](const std::stop_token& stopToken)
        {
            workerLoop(stopToken);
        });
    }

    void Worker::setMaterial(Material material)
    {
        postReset([material = material](Simulator& simulator)
        {
            simulator.setMaterial(material);
        });
    }

    void Worker::appendProgram(Program program)
    {
        auto shared = std::make_shared<Program>(std::move(program));
        post([shared](Simulator& simulator)
        {
            simulator.appendProgram(std::move(*shared));
        });
    }

    void Worker::clearPrograms()
    {
        postReset([](Simulator& simulator)
        {
            simulator.clearPrograms();
        });
    }

    void Worker::resetMaterial()
    {
        postReset([](Simulator& simulator)
        {
            simulator.resetMaterial();
        });
    }

    void Worker::setPlaying(const bool playing)
    {
        post([playing](Simulator& simulator)
        {
            simulator.setPlaying(playing);
        });
    }

    void Worker::reportShankCollision(Warning warning)
    {
        post([warning = std::move(warning)](Simulator& simulator) mutable
        {
            simulator.reportShankCollision(std::move(warning));
        });
    }

    void Worker::setSpeed(const double speed)
    {
        m_speed.store(std::clamp(speed, 0.01, 1000.0), std::memory_order_relaxed);
    }

    void Worker::finishNow()
    {
        {
            std::lock_guard lock(m_mutex);
            m_finishRequested.store(true, std::memory_order_relaxed);
        }
        m_condition.notify_one();
    }

    std::shared_ptr<const Snapshot> Worker::latestSnapshot() const
    {
        std::lock_guard lock(m_mutex);
        return m_latestSnapshot;
    }

    ToolPath Worker::takeCutterMoves()
    {
        std::lock_guard lock(m_mutex);
        ToolPath result = std::move(m_pendingCutterMoves);
        m_pendingCutterMoves.clear();
        return result;
    }

    std::size_t Worker::pendingCutterMoveCount() const
    {
        std::lock_guard lock(m_mutex);
        return m_pendingCutterMoves.size();
    }

    double Worker::speed() const
    {
        return m_speed.load(std::memory_order_relaxed);
    }

    void Worker::post(Command command)
    {
        {
            std::lock_guard lock(m_mutex);
            m_commands.push_back(std::move(command));
        }
        m_condition.notify_one();
    }

    void Worker::postReset(Command command)
    {
        {
            std::lock_guard lock(m_mutex);
            ++m_resetRevision;
            m_pendingCutterMoves.clear();
            m_latestSnapshot.reset();
            m_finishRequested.store(false, std::memory_order_relaxed);
            m_commands.push_back(std::move(command));
        }
        m_condition.notify_one();
    }

    void Worker::queueCutterMoves(ToolPath moves, const std::uint64_t resetRevision)
    {
        std::lock_guard lock(m_mutex);
        if (resetRevision != m_resetRevision)
        {
            return;
        }

        if (m_pendingCutterMoves.empty())
        {
            m_pendingCutterMoves = std::move(moves);
        }
        else
        {
            m_pendingCutterMoves.insert(
                m_pendingCutterMoves.end(),
                std::make_move_iterator(moves.begin()),
                std::make_move_iterator(moves.end())
            );
        }
    }

    void Worker::workerLoop(const std::stop_token stopToken)
    {
        using Clock = std::chrono::steady_clock;
        auto last = Clock::now();
        bool resetTimeline = false;
        std::uint64_t resetRevision = 0;

        const auto waitAndSwapQueues = [&](CommandQueue& commands, bool& finish)
        {
            std::unique_lock lock(m_mutex);
            const auto condition = [&]
            {
                return stopToken.stop_requested()
                    || !m_commands.empty()
                    || m_finishRequested.load(std::memory_order_relaxed);
            };
            if (m_simulator.playing())
            {
                m_condition.wait_for(lock, gc_playbackAdvanceDeltaTime, condition);
            }
            else
            {
                m_condition.wait(lock, condition);
                resetTimeline = true;
            }
            commands.swap(m_commands);
            resetRevision = m_resetRevision;
            finish = m_finishRequested.exchange(false, std::memory_order_relaxed);
        };

        const auto processCommands = [&](const CommandQueue& commands, const bool finish)
        {
            for (auto& command : commands)
            {
                command(m_simulator);
            }
            if (finish)
            {
                m_simulator.finishNow();
            }
        };

        const auto elapsedTime = [&]
        {
            const auto now = Clock::now();
            if (resetTimeline)
            {
                last = now;
                resetTimeline = false;
            }
            const auto realSeconds = std::min(
                std::chrono::duration<double>(now - last).count(),
                gc_maxFrameSeconds
            );
            return std::make_tuple(now, realSeconds);
        };

        const auto advancePlayback = [&](auto realSeconds)
        {
            if (m_simulator.playing())
            {
                m_simulator.advanceFixed(
                    realSeconds * m_speed.load(std::memory_order_relaxed),
                    gc_fixedStepSeconds
                );
                return true;
            }
            return false;
        };

        const auto flushCutterMoves = [&]
        {
            if (ToolPath cutterMoves = m_simulator.takeCutterMoves();
                !cutterMoves.empty())
            {
                queueCutterMoves(std::move(cutterMoves), resetRevision);
                return true;
            }
            return false;
        };

        const auto updateSnapshot = [&](auto dirty, auto now)
        {
            if (dirty)
            {
                auto snapshot = std::make_shared<Snapshot>(m_simulator.makeSnapshot());
                {
                    std::lock_guard lock(m_mutex);
                    if (resetRevision == m_resetRevision)
                    {
                        m_latestSnapshot = std::move(snapshot);
                    }
                }
            }
        };

        while (!stopToken.stop_requested())
        {
            CommandQueue commands;
            bool finish = false;
            waitAndSwapQueues(commands, finish);

            bool dirty = !commands.empty() || finish;
            processCommands(commands, finish);

            const auto [now, realSeconds] = elapsedTime();
            last = now;

            dirty |= advancePlayback(realSeconds);
            dirty |= flushCutterMoves();

            updateSnapshot(dirty, now);
        }
    }
}
