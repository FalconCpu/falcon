cmake_minimum_required(VERSION 3.10)
project(f32)

set(EXECUTABLE_OUTPUT_PATH "c:/Users/simon/falcon/bin")

add_executable(f32asm f32asm.c assemble.c lex.c util.c)
add_executable(f32dis f32dis.c disassemble.c util.c)
add_executable(f32sim f32sim.c disassemble.c util.c)
add_executable(process_font process_font.c)
add_executable(host_interface host_interface.c)


target_compile_options(f32asm PRIVATE -Wall -Wextra -Wno-microsoft-enum-forward-reference -Wno-deprecated-declarations)
target_compile_options(f32dis PRIVATE -Wall -Wextra -Wno-microsoft-enum-forward-reference -Wno-deprecated-declarations)
target_compile_options(f32sim PRIVATE -Wall -Wextra -Wno-microsoft-enum-forward-reference -Wno-deprecated-declarations)
target_compile_options(process_font PRIVATE -Wall -Wextra -Wno-microsoft-enum-forward-reference -Wno-deprecated-declarations)
target_compile_options(host_interface PRIVATE -Wall -Wextra -Wno-microsoft-enum-forward-reference -Wno-deprecated-declarations)
