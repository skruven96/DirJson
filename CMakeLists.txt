cmake_minimum_required(VERSION 3.0)
project(DirJson)

add_library(DirJson INTERFACE)

target_sources(DirJson
	INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/source/dirjson.h")

add_executable(DirTest tests/tests.c)

add_executable(DirPerf tests/perf_test.c)
