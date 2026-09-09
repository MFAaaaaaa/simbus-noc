cmake_minimum_required(VERSION 3.16)
project(rv64_manycore_sim LANGUAGES CXX)

option(MANYCORE_BUILD_TESTS "Build manycore simulator tests" ON)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../include/simbus/symmulcha.h")
  set(SIMBUS_NOC_DEFAULT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
else()
  set(SIMBUS_NOC_DEFAULT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../bus-test/simbus_noc")
endif()
set(SIMBUS_NOC_ROOT "${SIMBUS_NOC_DEFAULT_ROOT}" CACHE PATH
    "Path to the simbus_noc source tree")
set(RV64_ARCHSEM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../rv64-archsem" CACHE PATH
    "Path to the rv64-archsem source tree")

if(NOT EXISTS "${SIMBUS_NOC_ROOT}/CMakeLists.txt")
  message(FATAL_ERROR "SIMBUS_NOC_ROOT does not contain simbus_noc: ${SIMBUS_NOC_ROOT}")
endif()
if(NOT EXISTS "${RV64_ARCHSEM_ROOT}/CMakeLists.txt")
  message(FATAL_ERROR "RV64_ARCHSEM_ROOT does not contain rv64-archsem: ${RV64_ARCHSEM_ROOT}")
endif()

set(SIMBUS_NOC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SIMBUS_NOC_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("${SIMBUS_NOC_ROOT}" simbus-noc-build)
set(RV64_ARCHSEM_PATCHED_ROOT "${CMAKE_CURRENT_BINARY_DIR}/rv64-archsem-patched")
execute_process(
  COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_archsem.sh"
          "${RV64_ARCHSEM_ROOT}" "${RV64_ARCHSEM_PATCHED_ROOT}"
  RESULT_VARIABLE PREPARE_ARCHSEM_RESULT
  OUTPUT_VARIABLE PREPARE_ARCHSEM_OUTPUT
  ERROR_VARIABLE PREPARE_ARCHSEM_ERROR)
if(NOT PREPARE_ARCHSEM_RESULT EQUAL 0)
  message(FATAL_ERROR "Failed to prepare rv64-archsem:\n${PREPARE_ARCHSEM_OUTPUT}\n${PREPARE_ARCHSEM_ERROR}")
endif()
set(SANITIZER OFF CACHE BOOL "" FORCE)
add_subdirectory("${RV64_ARCHSEM_PATCHED_ROOT}" rv64-archsem-build)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(rv64archsem PRIVATE
    -include "${CMAKE_CURRENT_SOURCE_DIR}/include/manycore/compat_int128.hpp"
    -Wno-error)
endif()

add_library(manycore_sim
  src/memory_protocol.cpp
  src/shared_memory.cpp
  src/memory_controller.cpp
  src/atomic_core.cpp
  src/manycore_system.cpp
  src/program_image.cpp
  src/instruction_latency.cpp
  src/rv64_atomic_core.cpp
  src/rv64_manycore_system.cpp
)
add_library(manycore::sim ALIAS manycore_sim)

target_include_directories(manycore_sim PUBLIC include)
target_link_libraries(manycore_sim PUBLIC simbus::noc rv64archsem)
target_compile_features(manycore_sim PUBLIC cxx_std_20)
target_compile_options(manycore_sim PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)

add_executable(manycore_demo src/main.cpp)
target_link_libraries(manycore_demo PRIVATE manycore::sim)

add_executable(rv64_manycore_sim src/rv64_main.cpp)
target_link_libraries(rv64_manycore_sim PRIVATE manycore::sim)

if(MANYCORE_BUILD_TESTS)
  enable_testing()
  add_executable(manycore_tests tests/test_manycore.cpp)
  target_link_libraries(manycore_tests PRIVATE manycore::sim)
  add_test(NAME manycore_tests COMMAND manycore_tests)

  find_program(RISCV64_UNKNOWN_ELF_GCC riscv64-unknown-elf-gcc)
  find_program(RISCV64_UNKNOWN_ELF_OBJCOPY riscv64-unknown-elf-objcopy)
  find_program(RISCV64_UNKNOWN_ELF_NM riscv64-unknown-elf-nm)
  if(RISCV64_UNKNOWN_ELF_GCC AND RISCV64_UNKNOWN_ELF_OBJCOPY AND
     RISCV64_UNKNOWN_ELF_NM)
    set(TOHOST_EXAMPLE_DIR "${CMAKE_CURRENT_BINARY_DIR}/examples")
    set(TOHOST_EXAMPLE_BIN "${TOHOST_EXAMPLE_DIR}/tohost_smoke.bin")
    add_custom_command(
      OUTPUT "${TOHOST_EXAMPLE_BIN}"
      COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_rv64_example.sh"
              "${CMAKE_CURRENT_SOURCE_DIR}/examples/tohost_smoke.S"
              "${TOHOST_EXAMPLE_DIR}"
      DEPENDS examples/tohost_smoke.S examples/rv64_raw.ld
              scripts/build_rv64_example.sh
      VERBATIM)
    add_custom_target(rv64_tohost_example ALL DEPENDS "${TOHOST_EXAMPLE_BIN}")
    add_test(NAME rv64_tohost_example
      COMMAND $<TARGET_FILE:rv64_manycore_sim> "${TOHOST_EXAMPLE_BIN}" 4 0x80001000)

    set(VECTOR_EXAMPLE_BIN "${TOHOST_EXAMPLE_DIR}/vector_alu_smoke.bin")
    add_custom_command(
      OUTPUT "${VECTOR_EXAMPLE_BIN}"
      COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_rv64_example.sh"
              "${CMAKE_CURRENT_SOURCE_DIR}/examples/vector_alu_smoke.S"
              "${TOHOST_EXAMPLE_DIR}"
      DEPENDS examples/vector_alu_smoke.S examples/rv64_raw.ld
              scripts/build_rv64_example.sh
      VERBATIM)
    add_custom_target(rv64_vector_example ALL DEPENDS "${VECTOR_EXAMPLE_BIN}")
    add_test(NAME rv64_vector_example
      COMMAND $<TARGET_FILE:rv64_manycore_sim> "${VECTOR_EXAMPLE_BIN}" 4 0x80001000)

    set(VECTOR_MEMORY_EXAMPLE_BIN
        "${TOHOST_EXAMPLE_DIR}/vector_memory_smoke.bin")
    add_custom_command(
      OUTPUT "${VECTOR_MEMORY_EXAMPLE_BIN}"
      COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_rv64_example.sh"
              "${CMAKE_CURRENT_SOURCE_DIR}/examples/vector_memory_smoke.S"
              "${TOHOST_EXAMPLE_DIR}"
      DEPENDS examples/vector_memory_smoke.S examples/rv64_raw.ld
              scripts/build_rv64_example.sh
      VERBATIM)
    add_custom_target(rv64_vector_memory_example ALL
      DEPENDS "${VECTOR_MEMORY_EXAMPLE_BIN}")
    add_test(NAME rv64_vector_memory_example
      COMMAND $<TARGET_FILE:rv64_manycore_sim>
              "${VECTOR_MEMORY_EXAMPLE_BIN}" 4 0x80001000)

    set(VECTOR_CONTENTION_WORKLOADS
        vector_stream_private vector_stream_hotspot vector_stream_gap)
    set(VECTOR_CONTENTION_BINS)
    foreach(WORKLOAD IN LISTS VECTOR_CONTENTION_WORKLOADS)
      set(WORKLOAD_BIN "${TOHOST_EXAMPLE_DIR}/${WORKLOAD}.bin")
      add_custom_command(
        OUTPUT "${WORKLOAD_BIN}"
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_rv64_example.sh"
                "${CMAKE_CURRENT_SOURCE_DIR}/examples/${WORKLOAD}.S"
                "${TOHOST_EXAMPLE_DIR}"
        DEPENDS "examples/${WORKLOAD}.S" examples/rv64_raw.ld
                scripts/build_rv64_example.sh
        VERBATIM)
      list(APPEND VECTOR_CONTENTION_BINS "${WORKLOAD_BIN}")
    endforeach()
    add_custom_target(rv64_vector_contention_examples ALL
      DEPENDS ${VECTOR_CONTENTION_BINS})
    add_test(NAME rv64_vector_stream_private
      COMMAND $<TARGET_FILE:rv64_manycore_sim>
              "${TOHOST_EXAMPLE_DIR}/vector_stream_private.bin"
              --cores 2 --tohost 0x80001000 --max-ticks 2000000)
  else()
    message(STATUS "RISC-V bare-metal toolchain not found; skipping compiled example")
  endif()
endif()
