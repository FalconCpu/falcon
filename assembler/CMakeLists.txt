cmake_minimum_required(VERSION 3.10)
project(f32)

set(EXECUTABLE_OUTPUT_PATH "/home/simon/falcon/bin")

add_executable(f32asm f32asm.c assemble.c lex.c util.c)
add_executable(f32dis f32dis.c disassemble.c util.c)
add_executable(f32sim f32sim.c disassemble.c util.c)
add_executable(process_font process_font.c)
