#!/usr/bin/python3
import os
import sys


def run_test(test_name: str):
    """
    Runs a single unit test by compiling the test file and checking for errors.

    """

    (basename,extension) = test_name.split(".")
    outname = f"test_outputs/{basename}.out"
    expname = f"test_outputs/{basename}.exp"
    inname = f"testcases/{test_name}"

    # Split the source file into  and othe input and output sections
    # if test_name.startswith("run"):
    #     command = f"../bin/fpl --lex {inname} >{outname}"
    # elif test_name.startswith("parse"):
    #     command = f"../bin/fpl --parse {inname} >{outname}"
    # elif test_name.startswith("typecheck"):
    #     command = f"../bin/fpl --typecheck {inname} >{outname}"
    # elif test_name.startswith("codegen"):
    #     command = f"../bin/fpl --codegen {inname} >{outname}"
    # elif test_name.startswith("peephole"):
    #     command = f"../bin/fpl --peephole {inname} >{outname}"
    # else : 
    #     print("%-25s \033[0;31mUNDEFINED TYPE\033[0m"%test_name)
    #     return 0
    
    expected_output = []
    exp = open(expname,"w")
    with open(inname, 'r') as file:
        in_content = False  # Flag to track if we've crossed the marker line
        for line in file:
            if line.startswith("@------"):
                in_content = True  # Start collecting content after the marker
            elif in_content:
                expected_output.append(line.rstrip())  # Remove trailing newline and add the line
                exp.write(line)

    exp.close()

    # compile the files

    os_files = "src/uart.fpl"
    command = f"../bin/fpl {inname} {os_files} >{outname}"
    err = os.system(command)
    if err!=0:
        print("%-25s \033[0;31mCOMP FAIL\033[0m"%basename)
        return 0

    command = f"../bin/f32asm a.f32 >{outname}"
    err = os.system(command)
    if err!=0:
        print("%-25s \033[0;31mASM FAIL\033[0m"%basename)
        return 0

    command = f"../bin/f32sim rom.hex >{outname}"
    err = os.system(command)
    if err!=0:
        print("%-25s \033[0;31mSIM FAIL\033[0m"%basename)
        return 0


    with open(outname) as f:
        compiler_output = [line.rstrip() for line in f.readlines()]

    if compiler_output == expected_output:
        print("%-25s \033[0;32mPASS\033[0m"%basename)
        return 1
    else:
        print("%-25s \033[0;31mFAIL\033[0m"%basename)
        return 0


args = sys.argv


if len(args)==2:
    # run a single test case
    run_test(args[1])
else:
    # Run all test cases
    num_pass = 0
    num_tests = 0
    test_names = os.listdir("testcases")
    test_names.sort()

    for test_name in test_names:
        if test_name.endswith(".out"):
            continue
        num_tests += 1
        num_pass += run_test(test_name)

    print("%d/%d tests pass" % (num_pass, num_tests))
