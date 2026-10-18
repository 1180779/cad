//
// Created by Radosław Głasek on 18.10.2026
//

#include "cnc/NcProgram.hxx"

#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <fstream>
#include <tuple>

namespace cnc
{
    namespace
    {
        std::string trim(std::string value)
        {
            const auto notSpace = [](const unsigned char c)
            {
                return !std::isspace(c);
            };
            value.erase(
                value.begin(),
                std::ranges::find_if(value, notSpace)
            );
            value.erase(
                std::find_if(value.rbegin(), value.rend(), notSpace).base(),
                value.end()
            );
            return value;
        }

        /// @brief Remove line comments and nested, paired inline comments
        std::string stripComments(const std::string& line)
        {
            std::string result;
            result.reserve(line.size());

            constexpr auto endings = std::to_array<std::string_view>({";", "//"});
            constexpr auto inlineComments = std::to_array<std::tuple<std::string_view, std::string_view>>({
                std::make_tuple("/*", "*/"),
                std::make_tuple("(", ")")
            });
            std::vector<std::size_t> inlineCommentsStack;
            const auto matchCommentEnding = [&inlineComments, &inlineCommentsStack](
                const std::string_view remainder, std::size_t& i)
            {
                if (const auto ending = std::get<1>(inlineComments[inlineCommentsStack.back()]);
                    remainder.starts_with(ending))
                {
                    inlineCommentsStack.pop_back();
                    i += ending.size();
                    return true;
                }
                return false;
            };
            const auto matchCommentBeginning = [&inlineComments, &inlineCommentsStack](
                const std::string_view remainder, std::size_t& i)
            {
                bool commentBegins = false;
                for (std::size_t index = 0; index < inlineComments.size(); ++index)
                {
                    if (const auto beginning = std::get<0>(inlineComments[index]);
                        remainder.starts_with(beginning))
                    {
                        inlineCommentsStack.push_back(index);
                        i += beginning.size();
                        commentBegins = true;
                        break;
                    }
                }
                return commentBegins;
            };
            const auto matchEnding = [&endings](
                const std::string_view remainder)
            {
                return std::ranges::any_of(endings, [remainder](const auto ending)
                                           {
                                               return remainder.starts_with(ending);
                                           }
                );
            };


            for (std::size_t i = 0; i < line.size();)
            {
                const auto remaining = std::string_view(line).substr(i);
                if (!inlineCommentsStack.empty())
                {
                    // Inline comments nest: an opening delimiter inside a
                    // comment starts another comment of its own type.
                    if (matchCommentEnding(remaining, i))
                        continue;
                    if (matchCommentBeginning(remaining, i))
                        continue;
                    ++i;
                    continue;
                }

                if (matchEnding(remaining))
                    break;
                if (matchCommentBeginning(remaining, i))
                    continue;

                result.push_back(line[i]);
                ++i;
            }
            return result;
        }

        template <DiagnosticMessageType Type>
        void addDiagnostic(ParseResult& result, const int line, std::string message)
        {
            result.diagnostics.push_back({
                .line = line,
                .message = std::move(message),
                .type = Type
            });
        }

        [[maybe_unused]] constexpr auto addInfo = &addDiagnostic<DiagnosticMessageType::Info>;
        [[maybe_unused]] constexpr auto addWarning = &addDiagnostic<DiagnosticMessageType::Warning>;
        [[maybe_unused]] constexpr auto addError = &addDiagnostic<DiagnosticMessageType::Error>;

        /// @brief One `<letter><number>` G-code word, e.g. `X10.5`
        struct Word
        {
            /// @brief Source text of the word, including the letter
            std::string_view text;
            char letter{};
            double value{};
        };

        /// @brief Split a comment-free line into generic string words.
        ///
        /// This is the only place that walks the raw line. Everything above it
        /// works with the resulting `Word` list, so command handling does not
        /// need to look ahead at the next character
        std::optional<std::vector<Word>> tokenizeLine(
            const std::string& line,
            const int lineNumber,
            ParseResult& result
        )
        {
            std::vector<Word> words;

            const auto consumeWhitespace = [&line](std::size_t& index)
            {
                while (index < line.size()
                    && std::isspace(static_cast<unsigned char>(line[index])))
                {
                    ++index;
                }
            };
            const auto consumeAlpha = [&result, &lineNumber, &line](std::size_t& index)
            {
                if (!std::isalpha(static_cast<unsigned char>(line[index])))
                {
                    addError(
                        result,
                        lineNumber,
                        std::string("Unexpected character '") + line[index] + "'."
                    );
                    return std::make_tuple(false, '-', static_cast<std::size_t>(0));
                }
                const std::size_t wordBegin = index;
                const char letter = static_cast<char>(
                    std::toupper(static_cast<unsigned char>(line[index]))
                );
                ++index;
                return std::make_tuple(true, letter, wordBegin);
            };
            const auto consumePlus = [&line](const std::size_t& index, const char* lineEnd)
            {
                const char* numberStart = line.data() + index;
                if (numberStart != lineEnd && *numberStart == '+')
                {
                    ++numberStart;
                }
                return numberStart;
            };
            const auto parseNumber = [&result, &lineNumber](const char* numberStart, const char* lineEnd,
                                                            const char letter)
            {
                double value = 0.0;
                const auto [parseEnd, parseError] = std::from_chars(
                    numberStart,
                    lineEnd,
                    value
                );
                bool failed = false;
                if (parseError != std::errc{} || parseEnd == numberStart)
                {
                    addError(
                        result,
                        lineNumber,
                        std::string("Missing numeric value after '") + letter + "'."
                    );
                    failed = true;
                }
                return std::make_tuple(failed, parseEnd, value);
            };

            for (std::size_t index = 0; index < line.size();)
            {
                consumeWhitespace(index);
                if (index >= line.size())
                    break;
                // input file may start with a percent sign program marker
                // %G71 for mm 
                // %G70 for imperial
                if (line[index] == '%')
                {
                    ++index;
                    continue;
                }
                auto [alphaOk, letter, wordBegin] = consumeAlpha(index);
                if (!alphaOk)
                {
                    return std::nullopt;
                }
                consumeWhitespace(index);
                const char* const lineEnd = line.data() + line.size();
                const auto numberStart = consumePlus(index, lineEnd);
                const auto [parseFailed, parseEnd, value] = parseNumber(numberStart, lineEnd, letter);
                if (parseFailed)
                {
                    return std::nullopt;
                }

                index = static_cast<std::size_t>(parseEnd - line.data());
                words.push_back({
                    .text = std::string_view(
                        line.data() + wordBegin,
                        index - wordBegin
                    ),
                    .letter = letter,
                    .value = value,
                });
            }

            return words;
        }

        std::optional<Tool> inferToolFromName(const std::string& name)
        {
            return toolFromFilename(name);
        }
    }

    std::string_view toolTypeName(const ToolType type)
    {
        switch (type)
        {
        case ToolType::ball:
            return "ball";
        case ToolType::flat:
            return "flat";
        }
        return "unknown";
    }

    float Tool::cuttingLength() const
    {
        switch (type)
        {
        case ToolType::ball:
            return std::max(radius(), 0.1f);
        case ToolType::flat:
            return std::max(3.0f * diameter, 10.0f);
        }
        return 0.0f;
    }

    float Move::length() const
    {
        return (to - from).length();
    }

    double Move::durationSeconds() const
    {
        float speed{};
        switch (type)
        {
        case MoveType::G00:
            speed = gc_rapidFeed;
            break;
        case MoveType::G01:
            speed = feed > 0.0f ? feed : gc_defaultFeed;
            break;
        }
        return static_cast<double>(length()) / static_cast<double>(speed) * 60.0;
    }

    ParseResultFlags operator|(ParseResultFlags lhs, ParseResultFlags rhs)
    {
        return static_cast<ParseResultFlags>(
            static_cast<std::underlying_type_t<ParseResultFlags>>(lhs) |
            static_cast<std::underlying_type_t<ParseResultFlags>>(rhs)
        );
    }

    std::underlying_type_t<ParseResultFlags> operator&(ParseResultFlags lhs, ParseResultFlags rhs)
    {
        return static_cast<std::underlying_type_t<ParseResultFlags>>(static_cast<ParseResultFlags>(
            static_cast<std::underlying_type_t<ParseResultFlags>>(lhs) &
            static_cast<std::underlying_type_t<ParseResultFlags>>(rhs)
        ));
    }

    namespace
    {
        /// @brief Parse a dot-separated component such as `k06` or `f3`.
        std::optional<Tool> toolFromNameComponent(const std::string_view component)
        {
            if (component.size() < 2)
            {
                return std::nullopt;
            }

            ToolType type{};
            switch (std::tolower(static_cast<unsigned char>(component.front())))
            {
            case 'k':
                type = ToolType::ball;
                break;
            case 'f':
                type = ToolType::flat;
                break;
            default:
                return std::nullopt;
            }

            const auto diameterDigits = component.substr(1);
            const auto* const digitsBegin = diameterDigits.data();
            const auto* const digitsEnd = digitsBegin + diameterDigits.size();
            int diameter = 0;
            const auto [parseEnd, parseError] = std::from_chars(
                digitsBegin,
                digitsEnd,
                diameter
            );
            if (parseError != std::errc{} || parseEnd != digitsEnd || diameter <= 0)
            {
                return std::nullopt;
            }

            return Tool{
                .type = type,
                .diameter = static_cast<float>(diameter)
            };
        }
    }

    std::optional<Tool> toolFromFilename(const std::string_view filename)
    {
        const auto lastDot = filename.find_last_of('.');
        if (lastDot == std::string_view::npos)
        {
            return std::nullopt;
        }

        if (const auto tool = toolFromNameComponent(filename.substr(lastDot + 1)))
        {
            return tool;
        }

        const auto stem = filename.substr(0, lastDot);
        const auto previousDot = stem.find_last_of('.');
        if (previousDot == std::string_view::npos)
        {
            return std::nullopt;
        }

        return toolFromNameComponent(stem.substr(previousDot + 1));
    }

    ParseResult parseNcProgram(
        std::istream& input,
        std::string name,
        const std::optional<Tool> toolOverride
    )
    {
        ParseResult result;
        Program program;
        program.name = std::move(name);

        if (toolOverride)
        {
            program.tool = toolOverride.value();
        }
        else if (const auto inferred = inferToolFromName(program.name))
        {
            program.tool = inferred.value();
        }
        else
        {
            program.tool = Tool{};
            addWarning(
                result,
                0,
                "No .kXX/.fXX tool name found; using a 6 mm ball mill. "
                "Use the tool override to change it."
            );
        }

        double unitScale = 1.0;
        cadm::Vec3 position{};
        float feed = gc_defaultFeed;
        auto moveType = MoveType::G00;
        bool stop = false;
        int lineNumber = 0;
        std::string rawLine;
        const auto stripByteOrderMark = [&rawLine, &lineNumber]()
        {
            if (lineNumber == 1 && rawLine.size() >= 3
                && static_cast<unsigned char>(rawLine[0]) == 0xEF
                && static_cast<unsigned char>(rawLine[1]) == 0xBB
                && static_cast<unsigned char>(rawLine[2]) == 0xBF)
            {
                rawLine.erase(0, 3);
            }
        };

        const auto processWords = [&program, &unitScale, &moveType, &lineNumber, &result, &stop, &feed
            ](const std::vector<Word>& words, cadm::Vec3 &target)
        {
            bool error = false;
            bool sawCoordinate = false;
            for (const Word& word : words)
            {
                switch (word.letter)
                {
                case 'N':
                    // Line/instruction number; no effect on the path.
                    break;
                case 'G':
                    {
                        switch (const auto code = static_cast<int>(std::lround(word.value)))
                        {
                        case 0:
                            moveType = MoveType::G00;
                            break;
                        case 1:
                            moveType = MoveType::G01;
                            break;
                        case 70:
                            unitScale = 25.4;
                            program.unitType = UnitType::Imperial;
                            break;
                        case 71:
                            unitScale = 1.0;
                            program.unitType = UnitType::Metric;
                            break;
                        case 20:
                            unitScale = 25.4;
                            program.unitType = UnitType::Imperial;
                            break;
                        case 21:
                            unitScale = 1.0;
                            program.unitType = UnitType::Metric;
                            break;
                        default:
                            if (code == 2 || code == 3)
                            {
                                addWarning(
                                    result,
                                    lineNumber,
                                    "Arc motion (G02/G03) is not supported; "
                                    "the endpoint is treated as a linear move."
                                );
                            }
                            else if (code == 91)
                            {
                                addWarning(
                                    result,
                                    lineNumber,
                                    "Incremental coordinates (G91) are not supported; "
                                    "coordinates are treated as absolute."
                                );
                            }
                            // G40/G90 and other non-motion commands are ignored.
                            break;
                        }
                        break;
                    }
                case 'M':
                    if (static_cast<int>(std::lround(word.value)) == 30)
                    {
                        stop = true;
                    }
                    break;
                case 'S':
                    break;
                case 'F':
                    feed = static_cast<float>(word.value * unitScale);
                    if (!(feed > 0.0f))
                    {
                        addError(result, lineNumber, "Feed rate must be positive.");
                        error = true;
                    }
                    break;
                case 'X':
                    target.x = static_cast<cadm::cadf>(word.value * unitScale);
                    sawCoordinate = true;
                    break;
                case 'Y':
                    target.y = static_cast<cadm::cadf>(word.value * unitScale);
                    sawCoordinate = true;
                    break;
                case 'Z':
                    target.z = static_cast<cadm::cadf>(word.value * unitScale);
                    sawCoordinate = true;
                    break;
                default:
                    // Unknown command words are ignored to stay tolerant of
                    // machine-specific preambles.
                    break;
                }

                if (error)
                {
                    break;
                }
            }
            return std::make_tuple(error, sawCoordinate);
        };

        while (!stop && std::getline(input, rawLine))
        {
            ++lineNumber;
            stripByteOrderMark();
            const std::string line = trim(stripComments(rawLine));
            if (line.empty())
                continue;

            const auto words = tokenizeLine(line, lineNumber, result);
            if (!words)
                continue;

            cadm::Vec3 target = position;
            auto [lineError, sawCoordinate] = processWords(*words, target);
            if (lineError)
                continue;

            if (sawCoordinate)
            {
                if (target != position)
                {
                    Move move{
                        .from = position,
                        .to = target,
                        .type = moveType,
                        .feed = feed,
                        .tool = program.tool,
                    };
                    program.moves.push_back(move);
                }
                position = target;
            }
        }

        if (program.moves.empty())
        {
            addError(result, lineNumber, "No G00/G01 motion commands found.");
        }
        else
        {
            for (const Move& move : program.moves)
            {
                if (move.type == MoveType::G00)
                {
                    program.rapidDistance += static_cast<double>(move.length());
                }
                else
                {
                    program.cuttingDistance += static_cast<double>(move.length());
                }
            }
        }

        result.program = std::move(program);
        return result;
    }

    ParseResult parseNcProgramFile(
        const std::filesystem::path& path,
        const std::optional<Tool> toolOverride
    )
    {
        std::ifstream input(path, std::ios::binary);
        if (!input.is_open())
        {
            ParseResult result;
            addError(result, 0, "Could not open \"" + path.string() + "\".");
            return result;
        }

        ParseResult result = parseNcProgram(
            input,
            path.filename().string(),
            toolOverride
        );
        if (result.program)
        {
            result.program->sourcePath = path.string();
        }
        return result;
    }
}
