cmake_minimum_required(VERSION 3.10)
project(chip8-cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

set(SOURCES main.cpp chip8.cpp)
add_executable(chip8 ${SOURCES})

# https://github.com/cpp-best-practices/cppbestpractices/blob/master/02-Use_the_Tools_Available.md#gcc%E2%80%94clang
target_compile_options(chip8 PUBLIC
    -Wall -Wextra -pedantic -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    # -Wconversion
    # -Wsign-conversion
    -Wmisleading-indentation
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wnull-dereference
    -Wuseless-cast
    -Wdouble-promotion
    -Wformat=2
    # -Wlifetime
    -Wimplicit-fallthrough
)

target_compile_options(chip8 PUBLIC "$<$<CONFIG:Debug>:-g>")
target_compile_options(chip8 PUBLIC "$<$<CONFIG:Release>:-O3;-s>")

find_program(CPPCHECK cppcheck)
if(CPPCHECK)
    set(CMAKE_CXX_CPPCHECK ${CPPCHECK} --enable=all --suppress=missingInclude --suppress=unusedFunction)
else()
    message(AUTHOR_WARNING "cppcheck not found")
endif()

# find_program(CLANG_TIDY clang-tidy)
# if(CLANG_TIDY)
#     set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY})
# else()
#     message(AUTHOR_WARNING "clang-tidy not found")
# endif()

install(TARGETS chip8
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
target_link_libraries(chip8 ${SDL2_LIBRARIES})

