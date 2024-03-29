cmake_minimum_required(VERSION 3.9.1)
project(Slang)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3")

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
find_package(LLVM REQUIRED CONFIG)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)

message(STATUS "Found BISON ${BISON_VERSION}")
message(STATUS "Found FLEX ${FLEX_VERSION}")
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

BISON_TARGET(MyParser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cc DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.h)
FLEX_TARGET(MyScanner scanner.l ${CMAKE_CURRENT_BINARY_DIR}/scanner.cc)
ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${JSONCPP_INCLUDE_DIRS})
link_libraries(${JSONCPP_LIBRARIES})
add_definitions(${LLVM_DEFINITIONS})

set(SOURCE_FILES
        main.cc
        ${BISON_MyParser_OUTPUTS}
        ${FLEX_MyScanner_OUTPUTS}
        driver.h
        driver.cc
        absyn.h
        type.h
        type.cc
        IR.h
        IR.cc
        target_gen.h
        target_gen.cc
        debug.h
        optimize.h
        optimize.cc)
add_executable(Slang ${SOURCE_FILES})

llvm_map_components_to_libnames(llvm_libs all)
if (APPLE)
    target_link_libraries(Slang ${llvm_libs} ${JSONCPP_LIBRARIES})
endif ()
if (UNIX AND NOT APPLE)
    target_link_libraries(Slang LLVM ${JSONCPP_LIBRARIES})
endif ()

