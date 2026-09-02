cmake_minimum_required(VERSION 3.16)
project(chess_engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The engine library (source files are added as later tasks create them).
add_library(engine
    src/types.cpp
    src/board.cpp
    src/move.cpp
    src/movegen.cpp
    src/perft.cpp
    src/eval.cpp
    src/search.cpp
)
target_include_directories(engine PUBLIC src)

# Test executable.
add_executable(engine_tests
    tests/test_main.cpp
    tests/test_types.cpp
    tests/test_board.cpp
    tests/test_move.cpp
    tests/test_movegen.cpp
    tests/test_perft.cpp
    tests/test_eval.cpp
    tests/test_search.cpp
)
target_link_libraries(engine_tests PRIVATE engine)
target_include_directories(engine_tests PRIVATE third_party)

# Statically link the C++/GCC runtime so the exe has no external DLL deps.
# Without this, the exe can load an older libstdc++ from another MinGW on PATH
# (e.g. Git for Windows) and fail to start with 0xc0000139.
target_link_options(engine_tests PRIVATE -static)

enable_testing()
add_test(NAME engine_tests COMMAND engine_tests)
