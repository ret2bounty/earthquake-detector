software.elf: start.S main.c linker.ld
	riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 -T linker.ld -nostdlib -static -Wl,--no-warn-rwx-segments -o software.elf start.S main.c

clean:
	rm -f software.elf
