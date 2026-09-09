cmake_minimum_required(VERSION 3.16)
project(rv64-archsem)

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    Debug
    Release
)

if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING
        "Choose the type of build." FORCE)
endif()

option(SANITIZER "Enable Address/Undefined Sanitizer" ON)

function(enable_strict_warnings target)
    if (MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            /w14242  # conversion loss
            /w14254
            /w14263
            /w14265
            /w14287
            /we4289  # signed/unsigned mismatch
            /WX
        )
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wuninitialized
            -Wmaybe-uninitialized
            -Wundef
            -Wreturn-type
            -Wconversion
            -Wsign-conversion
            -Wfloat-conversion
            -Wdouble-promotion
            -Wodr
            -Werror
        )
    endif()
endfunction(enable_strict_warnings)

function(enable_sanitizers target)
    if (MSVC)
        message(WARNING "Sanitizers are not fully supported on MSVC")
        return()
    endif()

    if (SANITIZER)
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Debug>:-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all>
        )

        target_link_options(${target} PRIVATE
            $<$<CONFIG:Debug>:-fsanitize=address,undefined>
        )
    endif()
endfunction(enable_sanitizers)

function(setup_target_options target)
    target_compile_options(${target} PRIVATE
        $<$<CONFIG:Debug>:-O1 -g>
        $<$<CONFIG:Release>:-O3>
    )
    target_link_options(${target} PRIVATE
        $<$<CONFIG:Debug>:-O1 -g>
        $<$<CONFIG:Release>:-O3>
    )
    enable_strict_warnings(${target})
    enable_sanitizers(${target})
endfunction(setup_target_options)

file(GLOB_RECURSE SOFTFLOAT_LIB_SRC "softfloat/*.c")
add_library(softfloat_obj STATIC ${SOFTFLOAT_LIB_SRC})
target_include_directories(softfloat_obj PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/softfloat)
target_compile_options(softfloat_obj PRIVATE
    $<$<CONFIG:Debug>:-O1 -g>
    $<$<CONFIG:Release>:-O3>
)
target_link_options(softfloat_obj PRIVATE
    $<$<CONFIG:Debug>:-O1 -g>
    $<$<CONFIG:Release>:-O3>
)
add_library(softfloat STATIC $<TARGET_OBJECTS:softfloat_obj>)



file(GLOB_RECURSE LIB_SOURCES "riscv64/*.cpp")
add_library(rv64archsem STATIC ${LIB_SOURCES})
target_include_directories(rv64archsem PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/riscv64 ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(rv64archsem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(rv64archsem PRIVATE softfloat)
setup_target_options(rv64archsem)

set_target_properties(rv64archsem PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)
target_compile_definitions(rv64archsem PRIVATE LIB_BUILDING)
target_compile_features(rv64archsem PUBLIC cxx_std_20)


# Find all .cpp files in test directory
file(GLOB TEST_SOURCES "test/*.cpp")

# Create an executable for each test source file and link with the library
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} rv64archsem)
    target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    setup_target_options(${test_name})
    target_compile_features(${test_name} PRIVATE cxx_std_20)
endforeach()