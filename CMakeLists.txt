cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project("OctoCore")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)

if(EMSCRIPTEN)
add_definitions(-DEMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 --bind --memory-init-file 0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s NO_BROWSER=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --closure 0")
    if(CMAKE_BUILD_TYPE STREQUAL DEBUG)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s DISABLE_EXCEPTION_CATCHING=0")
    else(CMAKE_BUILD_TYPE STREQUAL DEBUG)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s DISABLE_EXCEPTION_CATCHING=1")
    endif(CMAKE_BUILD_TYPE STREQUAL DEBUG)
endif(EMSCRIPTEN)

enable_testing()

include_directories(.)

add_subdirectory(OctoCore)

# Tests #########################
add_executable(octocore_test
    test.cpp
    gtest/gtest.cpp
    gtest/gtest.h
    OctoCore/src/Command_test.cpp
    OctoCore/src/FieldHash_test.cpp
    OctoCore/src/State_test.cpp
    OctoCore/src/State_benchmark.cpp
)
target_link_libraries(octocore_test octocore)
if(EMSCRIPTEN)
target_include_directories(octocore_test SYSTEM PUBLIC ${EMSCRIPTEN_ROOT}/system/lib/libcxxabi/include/)
endif(EMSCRIPTEN)

add_test(OctoCoreTest octocore_test)

add_custom_target(testv COMMAND octocore_test --verbose)

file(GLOB vera_files
  *.cpp
  *.h
  deps/OctoCore/*.h
  deps/OctoCore/src/*.cpp
)
add_custom_target(vera_style ALL
  vera++
  -R L001 -R L001 -R L002 -R L003 -R L004 -R L005 -R L006
  -R T001 -R T002 -R T003 -R T004 -R T005 -R T006 -R T007
  -R T008 -R T009 -R T010 -R T012 -R T015 -R T016 -R T017
  -R T018 -R T019
  -P max-line-length=120
  ${vera_files}
)
