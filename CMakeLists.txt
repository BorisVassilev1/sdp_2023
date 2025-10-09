cmake_minimum_required(VERSION 4.0.0)
project(sdp_project)

set(COMPILE_ARGS 
	-std=c++26 -Wall -Wextra -Wpedantic --pedantic-errors -O3
	#-g -fsanitize=address
)

add_executable(dpda_test 
	DPDA/dpda_test.cpp
	DPDA/utils.cpp
	util/utils.cpp
)
target_compile_options(dpda_test PRIVATE ${COMPILE_ARGS})
target_link_options(dpda_test PRIVATE ${COMPILE_ARGS})
target_include_directories(dpda_test PRIVATE ../doctest.h .)
set_target_properties(dpda_test PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ../
)

#enable_testing()
add_test(NAME dpda_test
         COMMAND dpda_test 
		 DEPENDS dpda_test)

add_executable(cfg_test 
	DPDA/cfg_test.cpp
	DPDA/utils.cpp
	util/utils.cpp
)
target_compile_options(cfg_test PRIVATE ${COMPILE_ARGS})
target_link_options(cfg_test PRIVATE ${COMPILE_ARGS})
target_include_directories(cfg_test PRIVATE ../doctest.h .)
set_target_properties(cfg_test PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ../
)

add_executable(earley_test 
	DPDA/earley_test.cpp
	DPDA/utils.cpp
)
target_compile_options(earley_test PRIVATE ${COMPILE_ARGS})
target_link_options(earley_test PRIVATE ${COMPILE_ARGS})
target_include_directories(earley_test PRIVATE ../doctest.h .)
set_target_properties(earley_test PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ../
)

add_test(NAME cfg_test
         COMMAND cfg_test 
		 DEPENDS cfg_test)


add_custom_target(DPDA_check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS cfg_test dpda_test)

add_executable(language 
	DPDA/language.cpp
	DPDA/utils.cpp
	DPDA/token.cpp
	util/utils.cpp
	Regex/regexParser.cpp
	Regex/ambiguity.cpp
)
target_compile_options(language PRIVATE ${COMPILE_ARGS})
target_link_options(language PRIVATE ${COMPILE_ARGS})
target_include_directories(language PRIVATE ../doctest.h .)
set_target_properties(language PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ../
)


add_executable(regex 
	Regex/regex.cpp
	DPDA/utils.cpp
	DPDA/token.cpp
	Regex/regexParser.cpp
	Regex/ambiguity.cpp
	util/utils.cpp
)

target_compile_options(regex PRIVATE ${COMPILE_ARGS})
target_link_options(regex PRIVATE ${COMPILE_ARGS})
target_include_directories(regex PRIVATE ../doctest.h .)
set_target_properties(regex PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ../
)


add_executable(fst
	Regex/FST_test.cpp
	DPDA/utils.cpp
	DPDA/token.cpp
	Regex/regexParser.cpp
	Regex/ambiguity.cpp
	util/utils.cpp
)

target_compile_options(fst PRIVATE ${COMPILE_ARGS})
target_link_options(fst PRIVATE ${COMPILE_ARGS})
target_include_directories(fst PRIVATE ../doctest.h .)
set_target_properties(fst PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ../
)


add_executable(exam
	regex2.cpp
	DPDA/utils.cpp
	DPDA/token.cpp
	Regex/regexParser.cpp
	Regex/ambiguity.cpp
	util/utils.cpp
)

target_compile_options(exam PRIVATE ${COMPILE_ARGS})
target_link_options(exam PRIVATE ${COMPILE_ARGS})
target_include_directories(exam PRIVATE .)
set_target_properties(exam PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ../
)



add_executable(exam2
	regex3.cpp
	DPDA/utils.cpp
	DPDA/token.cpp
	Regex/regexParser.cpp
	Regex/ambiguity.cpp
	util/utils.cpp
)

target_compile_options(exam2 PRIVATE ${COMPILE_ARGS})
target_link_options(exam2 PRIVATE ${COMPILE_ARGS})
target_include_directories(exam2 PRIVATE .)
set_target_properties(exam2 PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ../
)

add_library(DPDA STATIC
	DPDA/language.cpp
	DPDA/utils.cpp
	DPDA/token.cpp
	util/utils.cpp
	Regex/regexParser.cpp
	Regex/ambiguity.cpp
)
target_compile_options(DPDA PRIVATE ${COMPILE_ARGS})
target_link_options(DPDA PRIVATE ${COMPILE_ARGS})
target_include_directories(DPDA PUBLIC .)
	



