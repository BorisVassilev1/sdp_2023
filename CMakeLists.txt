cmake_minimum_required(VERSION 3.5.0)
project(sdp_project)

set(COMPILE_ARGS -std=c++20 -Wall -Wextra -Wpedantic --pedantic-errors -g -fsanitize=address)

add_executable(dpda_test 
	DPDA/dpda_test.cpp
	DPDA/utils.cpp
)
target_compile_options(dpda_test PRIVATE ${COMPILE_ARGS})
target_link_options(dpda_test PRIVATE ${COMPILE_ARGS})
target_include_directories(dpda_test PRIVATE ../doctest.h .)
set_target_properties(dpda_test PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ../
)

enable_testing()
add_test(NAME dpda_test
         COMMAND dpda_test --build-target=dpda_test)



add_executable(language 
	DPDA/language.cpp
	DPDA/utils.cpp
	DPDA/token.cpp
)
target_compile_options(language PRIVATE ${COMPILE_ARGS})
target_link_options(language PRIVATE ${COMPILE_ARGS})
target_include_directories(language PRIVATE ../doctest.h .)
set_target_properties(language PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ../
)
