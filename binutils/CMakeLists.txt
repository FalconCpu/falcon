cmake_minimum_required(VERSION 3.10)

# Project name
project(f32asm C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add source files
set(ASM_SOURCES
    src/f32asm.c
    src/util.c
    src/token.c
    src/assemble.c
)

set(DIS_SOURCES
    src/f32dis.c
    src/util.c
    src/disassemble.c
)

set(SIM_SOURCES
    src/f32sim.c
    src/util.c
    src/execute.c
    src/disassemble.c
)

set(FILESYS_SOURCES
    src/filesys.c
    src/util.c
)

# Set output directories for all build types
set(EXECUTABLE_OUTPUT_PATH "c:/Users/simon/falcon/bin")

# Create executable
add_executable(f32asm ${ASM_SOURCES})
add_executable(f32dis ${DIS_SOURCES})
add_executable(f32sim ${SIM_SOURCES})
add_executable(host_interface src/host_interface.c)
add_executable(f32filesys ${FILESYS_SOURCES})

# Add compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

# Optional: Add libraries
# find_package(SomeLibrary REQUIRED)
# target_link_libraries(${PROJECT_NAME} PRIVATE SomeLibrary)
