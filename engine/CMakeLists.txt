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
    src/uci.cpp
)
target_include_directories(engine PUBLIC src)

if(EMSCRIPTEN)
    # WASM build: the same engine library, wrapped in a tiny C entry the browser
    # worker drives. No tests or native exe under Emscripten.
    add_executable(chesscoach src/wasm_api.cpp)
    target_link_libraries(chesscoach PRIVATE engine)
    set_target_properties(chesscoach PROPERTIES
        OUTPUT_NAME chesscoach
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../web/public/engine"
        LINK_FLAGS "-sMODULARIZE=1 -sEXPORT_ES6=1 -sENVIRONMENT=web,worker \
-sEXPORTED_FUNCTIONS=['_uci_command','_malloc','_free'] \
-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap'] -sALLOW_MEMORY_GROWTH=1")
else()
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
        tests/test_uci.cpp
    )
    target_link_libraries(engine_tests PRIVATE engine)
    target_include_directories(engine_tests PRIVATE third_party)

    # Statically link the C++/GCC runtime so the exe has no external DLL deps.
    # Without this, the exe can load an older libstdc++ from another MinGW on PATH
    # (e.g. Git for Windows) and fail to start with 0xc0000139.
    target_link_options(engine_tests PRIVATE -static)

    enable_testing()
    add_test(NAME engine_tests COMMAND engine_tests)

    # The UCI engine executable a chess GUI launches.
    add_executable(chess_engine src/main.cpp)
    target_link_libraries(chess_engine PRIVATE engine)
    target_include_directories(chess_engine PRIVATE src)
    # Same static-link fix as the test exe (avoid loading an older libstdc++ on PATH).
    target_link_options(chess_engine PRIVATE -static)
endif()
