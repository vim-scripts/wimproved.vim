cmake_minimum_required (VERSION 3.0.0)
project (wimproved)
add_library (wimproved SHARED wimproved.c)

set_target_properties(wimproved PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set_target_properties(wimproved PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
set_target_properties(wimproved PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
