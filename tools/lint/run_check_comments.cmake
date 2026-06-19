# driver for the check_comments target: collects tracked C/C++ sources and runs
# the lua checker over them. invoked via `cmake -DLUA=.. -DROOT=.. -P <this>`

execute_process(
        COMMAND git -C ${ROOT} ls-files *.cpp *.hpp *.h *.cc
        OUTPUT_VARIABLE tracked
        OUTPUT_STRIP_TRAILING_WHITESPACE)

string(REPLACE "\n" ";" files "${tracked}")

execute_process(
        COMMAND ${LUA} ${ROOT}/tools/lint/check_comments.lua ${files}
        WORKING_DIRECTORY ${ROOT}
        RESULT_VARIABLE rc)

if (NOT rc EQUAL 0)
    message(FATAL_ERROR "comment-style check failed")
endif ()
