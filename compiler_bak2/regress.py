#!/usr/bin/python3
import os
import sys


def run_test(test_name: str):
    """
    Runs a single unit test by compiling the test file and checking for errors.

    """

    (basename,extension) = test_name.split(".")
    outname = "test.out"
    expname = "test.expected"
    progname = "test.fpl"
    inname = f"testcases/{test_name}"

    # Split the source file into  and othe input and output sections
    if test_name.startswith("parse"):
        command = f"fplcomp -parse_only {progname} >{outname}"
    elif test_name.startswith("typecheck"):
        command = f"fplcomp -typecheck_only {progname} >{outname}"
    else:
        print("%-25s \033[0;31mUNDEFINED TYPE\033[0m"%test_name)
        return 0

    expected_output = []
    exp = open(expname,"w")
    prog = open(progname,"w")
    reached_divide = False
    with open(inname, 'r') as file:
        in_content = False  # Flag to track if we've crossed the marker line
        for line in file:
            if line.startswith("@------"):
                reached_divide = True
            elif reached_divide:                
                expected_output.append(line.rstrip())  # Remove trailing newline and add the line
                exp.write(line)
            else:
                prog.write(line)

    exp.close()
    prog.close()

    os.system(command)

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
