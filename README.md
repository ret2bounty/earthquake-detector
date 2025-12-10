# Earthquake Detector
A simple, low-power earthquake detector using a RISC-V microcontroller and accelerometer for use in remote areas. It can be simulated in Renode.

# Compilation and Simulation
1. Compile the `main.c` and `start.S` files into `software.elf`:
   `riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 -Ttext=0x80000000 -nostdlib -o /mnt/c/Users/click/computerarchitecture/project/software.elf start.s main.c`
2. Run the simulation using Renode Monitor by running the command `include @simulation.resc`.
