//
// Created by Radosław Głasek on 18.10.2026
//

#ifndef CAD_NCPROGRAM_HXX
#define CAD_NCPROGRAM_HXX

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <cad_math/Vec3.hxx>

namespace cnc
{
    /// @brief Rapid move feed; in mm/min
    inline constexpr float gc_rapidFeed = 20000.0f;

    /// @brief Default cutting feed; in mm/min
    inline constexpr float gc_defaultFeed = 15000.0f;

    /// @brief Geometry of the cutting tool
    enum class ToolType
    {
        ball,
        flat,
    };

    [[nodiscard]] std::string_view toolTypeName(ToolType type);

    /// @brief Tool description (diameter in millimetres)
    struct Tool
    {
        ToolType type{ToolType::ball};
        /// @brief Cutting diameter in mm
        float diameter{6.0f};

        [[nodiscard]] float radius() const
        {
            return diameter * 0.5f;
        }

        /// @brief Height of the cutting portion above the tool tip, in mm
        ///
        /// Material touching the tool above this height is hit by the
        /// non-cutting shank. This is an advisory collision model: a ball mill
        /// uses the hemisphere (one diameter), a flat mill a conservative flute
        /// length
        [[nodiscard]] float cuttingLength() const;
    };

    enum class MoveType
    {
        /* rapid move */
        G00,
        /* cutting feed */
        G01
    };

    /// @brief One linear move of the machine 
    /// @note Only G00/G01 moves are supported
    struct Move
    {
        cadm::Vec3 from{};
        cadm::Vec3 to{};
        /// @brief Move type
        MoveType type{MoveType::G01};
        /// @brief Feed rate in mm/min
        float feed{gc_defaultFeed};
        Tool tool{};
        /// @brief Index of the source program instruction
        std::size_t programIndex{0};

        [[nodiscard]] float length() const;

        /// @brief Simulated duration of the move in seconds
        [[nodiscard]] double durationSeconds() const;
    };

    enum class UnitType
    {
        Metric,
        Imperial,
    };

    /// @brief A parsed toolpath program
    struct Program
    {
        std::string name;
        std::string sourcePath;
        Tool tool{};
        std::vector<Move> moves;
        double cuttingDistance{};
        double rapidDistance{};
        UnitType unitType{UnitType::Metric};
    };

    enum class DiagnosticMessageType
    {
        Info,
        Warning,
        Error
    };

    /// @brief Parser feedback for one source line
    struct Diagnostic
    {
        int line{};
        std::string message;
        DiagnosticMessageType type{DiagnosticMessageType::Info};
    };

    enum class ParseResultFlags
    {
        /* treat warnings as errors */
        WError = 1,
    };

    ParseResultFlags operator|(ParseResultFlags lhs, ParseResultFlags rhs);
    std::underlying_type_t<ParseResultFlags> operator&(ParseResultFlags lhs, ParseResultFlags rhs);

    struct ParseResult
    {
        ParseResultFlags flags{ParseResultFlags::WError};
        std::optional<Program> program;
        std::vector<Diagnostic> diagnostics;

        [[nodiscard]] bool ok() const
        {
            if (!program.has_value())
            {
                return false;
            }
            /* ok when program contains no errors */
            return std::ranges::none_of(diagnostics,
                                        [this](const Diagnostic& diagnostic)
                                        {
                                            return diagnostic.type == DiagnosticMessageType::Error ||
                                            (flags & ParseResultFlags::WError && diagnostic.type ==
                                                DiagnosticMessageType::Warning);
                                        }
            );
        }
    };

    /// @brief Infer the tool from the naming convention
    [[nodiscard]] std::optional<Tool> toolFromFilename(std::string_view filename);

    /// @brief Parse an NC file from a stream
    /// @param name Display name, also used to infer the tool when
    /// @p toolOverride is empty
    /// @param toolOverride When set, takes precedence over the filename hint
    [[nodiscard]] ParseResult parseNcProgram(
        std::istream& input,
        std::string name,
        std::optional<Tool> toolOverride = std::nullopt
    );

    /// @brief Parse an NC file from the drive
    [[nodiscard]] ParseResult parseNcProgramFile(
        const std::filesystem::path& path,
        std::optional<Tool> toolOverride = std::nullopt
    );
}

#endif //CAD_NCPROGRAM_HXX
