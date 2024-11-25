import os
import subprocess
import argparse

def run_simulation(test_file):
  """
  Runs the simulation and emulation for a given test case.

  Args:
    test_file: Path to the test case file (with suffix .f32).
    vvp_path: Path to the vvp.exe simulator executable.
    f32sim_path: Path to the f32sim.exe emulator program.

  """
  # Run the assembler
  rtl_log = "rtl_regs.log"
  vvp_cmd = ["f32asm.exe", test_file]
  subprocess.run(vvp_cmd, check=True)

  # Run RTL simulation
  rtl_log = "rtl_reg.log"
  vvp_cmd = ["vvp.exe", "a.out"] 
  vvplog = open("vvp.log","w")
  subprocess.run(vvp_cmd, check=True, stdout=vvplog)

  # Run CPU emulation
  sim_log = "sim_reg.log"
  sim_cmd = ["f32sim.exe", "asm.hex"]
  subprocess.run(sim_cmd, check=True,  stdout=vvplog)

  # color escape codes
  RED = "\033[1;31m"
  GREEN = "\033[1;32m"
  RESET = "\033[0m"  # Reset color


  # Compare log files
  with open(rtl_log, "r") as f1, open(sim_log, "r") as f2:
    rtl_lines = f1.readlines()
    sim_lines = f2.readlines()

  with open("rtl_uart.log", "r") as f1, open("sim_uart.log", "r") as f2:
    rtl_uart_lines = f1.readlines()
    sim_uart_lines = f2.readlines()

  padding = " " * (40-len(test_file))

  if len(rtl_uart_lines) != 0:
    # if the test wrote to uart, then compare uart logs
    
    if rtl_uart_lines == sim_uart_lines:
      print(f"{test_file}{padding} {GREEN}PASS UART{RESET}")
    else:
      print(f"{test_file}{padding} {RED}FAIL UART{RESET}")
  
  else:
    # if the test did not write to uart, then compare register logs files
    if rtl_lines == sim_lines:
      print(f"{test_file}{padding} {GREEN}PASS{RESET}")
    else:
      print(f"{test_file}{padding} {RED}FAIL{RESET}")


if __name__ == "__main__":

  # Define argument parser
  parser = argparse.ArgumentParser(description="Run CPU model regression tests.")
  parser.add_argument("test_file", nargs="?", type=str, help="Path to a specific test case file (.f32).")
  args = parser.parse_args()

  if args.test_file:
    run_simulation(args.test_file)    
  else:
    # Loop through all test case files
    test_dir = "testcases"
    for filename in os.listdir("testcases"):
        if filename.endswith(".f32"):
            test_file = os.path.join(test_dir, filename)
            run_simulation(test_file)