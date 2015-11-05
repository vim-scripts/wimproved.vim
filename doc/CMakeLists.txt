cmake_minimum_required (VERSION 2.8.7)
project (wimproved)
add_library (wimproved SHARED wimproved.c)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR})
