#!/usr/bin/env lua
-- comment-style checker for the cad codebase
--
-- rules (see memory/comment conventions):
--   * inline comments (//) start lowercase and end without a period
--   * doc comments (///) start uppercase and end without a period
--   * proper nouns (Bezier, GPU, ...) may start an inline comment uppercase
--   * marker comments (TO_DO, NOLINT, ReSharper, ...) are ignored
--
-- usage: lua check_comments.lua <file> [<file> ...]
-- exits 1 if any violation is found; prints "path:line: message" per hit
--
-- heuristic, not a parser: it skips // inside strings and URLs with a light
-- check, and may miss or over-report in pathological cases

-- proper nouns allowed to start an inline comment with an uppercase letter
local PROPER_NOUNS = {
    Bernstein = true,
    De = true,
    Bezier = true,
    GPU = true,
    CPU = true,
    VBO = true,
    EBO = true,
    VAO = true,
    Qt = true,
    OpenGL = true,
    GL = true,
    NDC = true,
    ID = true,
    IDs = true,
    CMake = true,
    TBB = true,
    SoA = true,
    AABB = true,
    X = true,
    Y = true,
    Z = true,
    W = true,
    Phong = true,
}

-- first tokens that mark a non-prose comment; the whole comment is skipped
local SKIP_MARKERS = {
    TODO = true,
    FIXME = true,
    HACK = true,
    XXX = true,
    NOTE = true,
    NOLINT = true,
    ["NOLINTNEXTLINE"] = true,
    ReSharper = true,
    ["clang-format"] = true,
    ["clang-tidy"] = true,
    -- CLion file-header boilerplate ("// Created by ... on ...")
    Created = true,
}

local function trim(s)
    return (s:gsub("^%s+", ""):gsub("%s+$", ""))
end

-- first whitespace-delimited token, stripped of surrounding punctuation
local function first_word(body)
    local w = body:match("^%s*([%w%-_]+)")
    return w
end

-- find the first real // comment on a line; returns slashCount, body or nil
-- skips // that sits inside a string literal or right after ':' (URLs)
local function find_comment(line)
    local in_string = false
    local i = 1
    local n = #line
    while i <= n do
        local c = line:sub(i, i)
        if c == '"' and line:sub(i - 1, i - 1) ~= "\\" then
            in_string = not in_string
        elseif not in_string and c == "/" and line:sub(i + 1, i + 1) == "/" then
            if line:sub(i - 1, i - 1) ~= ":" then
                local slashes = #(line:match("^/+", i))
                local body = trim(line:sub(i + slashes))
                return slashes, body
            end
        end
        i = i + 1
    end
    return nil
end

-- first non-space character of a comment body, or nil if blank; used for the
-- start-case rule. when it is not a letter (math/code fragments like "(O + t*Dir)")
-- the rule does not apply
local function first_char(s)
    return s:match("^%s*(%S)")
end

local function ends_with_period(body)
    -- allow ellipsis
    if body:sub(-3) == "..." then return false end
    return body:sub(-1) == "."
end

local violations = 0
local function report(path, lineno, msg)
    io.write(string.format("%s:%d: %s\n", path, lineno, msg))
    violations = violations + 1
end

local function check_file(path)
    local f = io.open(path, "r")
    if not f then return end
    local lines = {}
    for l in f:lines() do lines[#lines + 1] = l end
    f:close()

    -- precompute the comment kind (slash count) and body per line; preprocessor
    -- lines (#endif // GUARD, #else // ..., etc.) carry label comments, not prose,
    -- so they are skipped and excluded from block adjacency
    local kinds, bodies = {}, {}
    for lineno, line in ipairs(lines) do
        if not line:match("^%s*#") then
            kinds[lineno], bodies[lineno] = find_comment(line)
        end
    end

    for lineno = 1, #lines do
        local slashes = kinds[lineno]
        local body = bodies[lineno]
        -- block boundaries: a run of consecutive lines sharing the same comment kind
        local is_block_start = slashes and (kinds[lineno - 1] ~= slashes)
        local is_block_end = slashes and (kinds[lineno + 1] ~= slashes)

        if slashes and body ~= "" then
            local fw = first_word(body)
            local is_marker = fw and SKIP_MARKERS[fw]

            if not is_marker then
                local doc = slashes >= 3

                -- trailing period applies only to the last line of the block, so an
                -- intermediate sentence may keep its period
                if is_block_end and ends_with_period(body) then
                    report(path, lineno,
                        (doc and "doc" or "inline") .. " comment should not end with a period")
                end

                -- start-case applies only to the first line of a block
                if is_block_start then
                    local text = body
                    -- doc: drop a leading @brief tag; skip other @tag lines entirely
                    if doc then
                        if text:match("^@brief%s+") then
                            text = trim(text:gsub("^@brief%s+", ""))
                        elseif text:match("^@") then
                            text = "" -- @param/@return/... not subject to start-case
                        end
                    end
                    -- only enforce case when the comment opens with a letter; a
                    -- leading bracket/symbol/digit marks a math or code fragment
                    local a = first_char(text)
                    if a and a:match("%a") then
                        if doc and a:match("%l") then
                            report(path, lineno, "doc comment should start with an uppercase letter")
                        elseif (not doc) and a:match("%u") and not (fw and PROPER_NOUNS[fw]) then
                            report(path, lineno, "inline comment should start lowercase (or be a proper noun)")
                        end
                    end
                end
            end
        end
    end
end

local args = { ... }
if #args == 0 then
    io.stderr:write("usage: lua check_comments.lua <file> [<file> ...]\n")
    os.exit(2)
end
for _, path in ipairs(args) do
    check_file(path)
end

if violations > 0 then
    io.stderr:write(string.format("\n%d comment-style violation(s)\n", violations))
    os.exit(1)
end
os.exit(0)
