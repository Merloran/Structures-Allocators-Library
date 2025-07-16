cmake_minimum_required(VERSION 3.26)

set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

set(targetName Serrate)
project(${targetName})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(xxHash CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)

file(GLOB_RECURSE sourceFiles
     "${CMAKE_CURRENT_SOURCE_DIR}/Serrate/*.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/Serrate/*.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/Serrate/*.hpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/Serrate/*.inl")

list(REMOVE_ITEM sourceFiles "${CMAKE_CURRENT_SOURCE_DIR}/Serrate/main.cpp")

add_library(${targetName} STATIC ${sourceFiles})

foreach(sourceFile IN ITEMS ${sourceFiles})
    get_filename_component(sourcePath "${sourceFile}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/Serrate" "" groupPath "${sourcePath}")
    string(REPLACE "/" "\\" groupPath "${groupPath}")
    source_group("Project\\${groupPath}" FILES "${sourceFile}")
endforeach()

target_link_libraries(${targetName} PRIVATE spdlog::spdlog)
target_link_libraries(${targetName} PRIVATE magic_enum::magic_enum)
target_link_libraries(${targetName} PRIVATE xxHash::xxhash)

if (MSVC)
    target_compile_options(${targetName} PRIVATE
						   /utf-8
						   /W4
						   /WX
						   /Qspectre-
						   /external:W0
						   /external:anglebrackets)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${targetName} PRIVATE 
						   -Wall
						   -Wextra
						   -Werror
						   -include "$(IntDir)cmake_pch.hxx")
endif()

target_include_directories(${targetName} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})