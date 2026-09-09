cmake_minimum_required(VERSION 3.16)
project(simbus_noc LANGUAGES CXX)

option(SIMBUS_NOC_BUILD_TESTS "Build simbus_noc tests" ON)
option(SIMBUS_NOC_BUILD_EXAMPLES "Build simbus_noc examples" ON)

add_library(simbus_noc
    src/routetable.cpp
    src/symmulcha.cpp
)

add_library(simbus::noc ALIAS simbus_noc)

target_include_directories(simbus_noc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(simbus_noc PUBLIC cxx_std_17)

target_compile_options(simbus_noc PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

if(SIMBUS_NOC_BUILD_TESTS)
    enable_testing()
    add_executable(simbus_noc_tests
        tests/test_main.cpp
        tests/test_routetable.cpp
        tests/test_basic_transfer.cpp
        tests/test_congestion.cpp
        tests/test_stress.cpp
    )
    target_link_libraries(simbus_noc_tests PRIVATE simbus::noc)
    add_test(NAME simbus_noc_tests COMMAND simbus_noc_tests)
endif()

if(SIMBUS_NOC_BUILD_EXAMPLES)
    add_executable(noc_benchmark
        examples/noc_benchmark.cpp
    )
    target_link_libraries(noc_benchmark PRIVATE simbus::noc)
endif()
