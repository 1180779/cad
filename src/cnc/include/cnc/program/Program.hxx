//
// Created by Radosław Głasek on 18.10.2026
//

#ifndef CAD_NCPROGRAM_HXX
#define CAD_NCPROGRAM_HXX

#include "Move.hxx"
#include "Tool.hxx"

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cnc
{
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
        ToolPath moves;
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

    [[nodiscard]] std::optional<Tool> toolFromFilename(std::string_view filename);

    /// @param name Display name
    /// @param toolOverride When set, takes precedence over the filename
    /// convention
    [[nodiscard]] ParseResult parseProgram(
        std::istream& input,
        std::string name,
        std::optional<Tool> toolOverride = std::nullopt
    );

    [[nodiscard]] ParseResult parseProgramFile(
        const std::filesystem::path& path,
        std::optional<Tool> toolOverride = std::nullopt
    );
}

#endif //CAD_NCPROGRAM_HXX
