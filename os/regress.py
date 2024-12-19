#!/usr/bin/python3
import os
import sys

RED = "\033[0;31m"
GREEN = "\033[0;32m"
DEFAULT = "\033[0m"

def run_test(test_name: str):
    """
    Runs a single unit test by compiling the test file and checking for errors.

    """

    (basename,extension) = test_name.split(".")
    srcname = f"testcase_outputs/{basename}.fpl"
    outname = f"testcase_outputs/{basename}.out"
    expname = f"testcase_outputs/{basename}.exp"
    inname = f"testcases/{test_name}"

    # Split the source file into the executable and expected sections
    expected_output = []
    with open(inname, 'r') as file, open(expname,"w") as exp, open(srcname,"w") as src:
        after_line = False  # Flag to track if we've crossed the marker line
        for line in file:
            if line.startswith("@------"):
                after_line = True  
            elif after_line:
                expected_output.append(line.rstrip())  # Remove trailing newline and add the line
                exp.write(line)
            else:
                src.write(line)

    command = f"fplcomp.exe {srcname} > {outname}"
    err = os.system(command)
    if err!=0:
        print(f"%-30s {RED}FAIL-COMPILE{DEFAULT}"%basename)
        return 0

    asmcommand = f"f32asm.exe src/start.f32 output.f32 "
    err = os.system(asmcommand)
    if err!=0:
        print(f"%-30s {RED}FAIL-ASM{DEFAULT}"%basename)
        return 0

    simcommand = f"f32sim.exe asm.hex > {outname}"
    err = os.system(simcommand)
    if err!=0:
        print(f"%-30s {RED}FAIL-SIM{DEFAULT}"%basename)
        return 0

    with open(outname) as f:
        compiler_output = [line.rstrip() for line in f.readlines()]
        
    if compiler_output == expected_output:
        print(f"%-30s {GREEN}PASS{DEFAULT}"%basename)
        return 1
    else:
        print(f"%-30s {RED}FAIL{DEFAULT}"%basename)
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
        if test_name.endswith(".fpl"):
            num_tests += 1
            num_pass += run_test(test_name)

    print("%d/%d tests pass" % (num_pass, num_tests))
