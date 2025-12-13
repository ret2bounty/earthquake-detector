# Compilation and Simulation
1. Compile the `main.c` and `start.S` files into `software.elf` using a RISC-V cross-compiler:
   `make` or `make software.elf`
2. Run the simulation using Renode Monitor by running the command `include @simulation.resc`


# Earthquake Detector
A simple, low-power earthquake detector using a RISC-V microcontroller and accelerometer for use in remote areas. It can be simulated in Renode.

## Project Structure
```
earthquake-detector/
├── start.S          # RISC-V assembly startup code (stack init, BSS clearing, entry point)
├── main.c           # Main logic (sensor reading, STA/LTA detection, alert system (via GPIO (pending))) here
├── earthquake.repl  # Renode platform definition that includes details of the hardware (CPU, RAM, UART, GPIO, accelerometer sensor)
├── simulation.resc  # Renode script that loads platform and ELF, and then starts simulation
├── linker.ld        # Defines all layout for the final executable
├── Makefile         # To build C and Assembly code into a RISC-V ELF Executable
├── software.elf     # (generated) Compiled binary loaded into Renode simulation
└── README.md        # Self-explanatory .-.
```

## Compilation and Simulation
1. Compile the `main.c` and `start.S` files into `software.elf` using a RISC-V cross-compiler:
   ```sh
   make # or `make software.elf`
   ```
2. Run the simulation using Renode:
   ```sh
   renode
   ```
   Then in the Renode console:
   ```
   include @simulation.resc
   ```

## Hardware Memory Map
| Address      | Peripheral           | Description                          |
|--------------|----------------------|--------------------------------------|
| `0x10000000` | UART (NS16550)       | Serial output for alerts             |
| `0x20000000` | Accelerometer Sensor | 3-axis sensor with earthquake sim    |
| `0x30000000` | GPIO (pending)       | LED indicators for alert status      |
| `0x80000000` | RAM (64KB)           | Program code and data                |

## Detection Algorithm
Uses **STA/LTA** (Short-Term Average / Long-Term Average) ratio - a real seismological method for earthquake detection. Classifies events into Minor, Moderate, Severe, and Extreme levels based on magnitude thresholds.