cmake_minimum_required(VERSION 3.16)
project(proclens C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(proclens_tests
    src/ds/structures.c
    src/sim/model.c
    src/sim/simulation.c
    src/analysis/report.c
    src/analysis/groups.c
    src/analysis/adaptive.c
    src/analysis/advisor.c
    src/collector/snapshot.c
    src/collector/collector_win.c
    src/collector/collector_mac.c
    tests/test_main.c
)

target_include_directories(proclens_tests PRIVATE src/ds src/sim src/analysis src/collector tests)
target_compile_options(proclens_tests PRIVATE -Wall -Wextra -Wpedantic -Werror -g)

enable_testing()
add_test(NAME phase0_foundation_tests COMMAND proclens_tests)

add_executable(proclens_cli
    src/ds/structures.c src/sim/model.c src/sim/simulation.c
    src/analysis/report.c src/analysis/groups.c src/analysis/adaptive.c
    src/analysis/advisor.c src/collector/snapshot.c
    src/collector/collector_win.c src/collector/collector_mac.c src/cli/main.c
)
target_include_directories(proclens_cli PRIVATE src/ds src/sim src/analysis src/collector)
target_compile_options(proclens_cli PRIVATE -Wall -Wextra -Wpedantic -Werror -g)

add_executable(proclens_server
    src/ds/structures.c src/sim/model.c src/sim/simulation.c
    src/analysis/report.c src/analysis/groups.c src/analysis/adaptive.c
    src/analysis/advisor.c src/collector/snapshot.c
    src/collector/collector_win.c src/collector/collector_mac.c
    src/server/http_runtime.c src/server/api.c src/server/main.c
)
target_include_directories(proclens_server PRIVATE src/ds src/sim src/analysis src/collector src/server)
target_compile_options(proclens_server PRIVATE -Wall -Wextra -Wpedantic -Werror -g)
if(WIN32)
    target_link_libraries(proclens_server PRIVATE ws2_32)
endif()

