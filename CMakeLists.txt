cmake_minimum_required(VERSION 3.5.0)
project(sdp_project)

set(COMPILE_ARGS -std=c++20 -Wall -Wextra -Wpedantic -g --pedantic-errors -O3)


add_executable(dpda_test 
	DPDA/cfg.cpp 
	DPDA/dpda.cpp 
	DPDA/dpda_test.cpp
	DPDA/utils.cpp
)

target_compile_options(dpda_test PRIVATE ${COMPILE_ARGS})
target_link_options(dpda_test PRIVATE ${COMPILE_ARGS})
set_target_properties(dpda_test PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ../
)
