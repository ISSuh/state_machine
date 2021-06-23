cmake_minimum_required (VERSION 3.8)

project ("state_machine")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
	add_compile_options(-g -Wall)

	set(DEPENDENCY_LIB pthread)
endif()

set(EXAMPLE_DIR ${CMAKE_SOURCE_DIR}/example)

add_executable (sequence_example ${EXAMPLE_DIR}/sequence_example.cc)
target_link_libraries(sequence_example ${DEPENDENCY_LIB})

add_executable (concurreny_example ${EXAMPLE_DIR}/concurreny_example.cc)
target_link_libraries(concurreny_example ${DEPENDENCY_LIB})
