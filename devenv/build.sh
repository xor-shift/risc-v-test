#!/usr/bin/env bash

#riscv64-unknown-elf-gcc -mabi=ilp32 -march=rv32i -nostdlib -nostartfiles -Tlink.ld prog.s -o prog.o
#riscv64-unknown-elf-g++ -mabi=lp64 -march=rv64im -nostdlib -nostartfiles -Tlink.ld -std=c++2b startup.s startup.cpp prog.cpp -o prog.elf
#riscv64-unknown-elf-g++ -mabi=lp64d -march=rv64id -lc -lm -lnosys -Tlink.ld -std=c++2b startup.s startup.cpp prog.cpp -o prog.elf

#ABI=ilp32
#ARCH=rv32imc
#LDARGS="-b elf32-littleriscv -m elf32lriscv"

ABI=lp64d
ARCH=rv64imfdc
# -b elf64-littleriscv -m elf64lriscv
LDARGS="-L /usr/riscv64-unknown-elf/lib -lc -lnosys -T ../link.ld"

ASARGS=""
#CXXARGS="-nostdlib -nostartfiles -std=c++2b -Wall -Wno-sign-compare -O3"
CXXARGS="-ffunction-sections -Wl,--gc-sections -std=c++2b -Wall -Wno-sign-compare -O3"

[[ -d build ]] || mkdir build
riscv64-unknown-elf-as -mabi=$ABI -march=$ARCH $ASARGS -c startup.s -o build/startup.o
riscv64-unknown-elf-g++ -mabi=$ABI -march=$ARCH $CXXARGS -c entry.cpp -o build/entry.o
riscv64-unknown-elf-g++ -mabi=$ABI -march=$ARCH $CXXARGS -c special.cpp -o build/special.o
riscv64-unknown-elf-g++ -mabi=$ABI -march=$ARCH $CXXARGS -c prog.cpp -o build/prog.o

cd build
riscv64-unknown-elf-gcc $LDARGS startup.o entry.o special.o prog.o -o prog.elf
riscv64-unknown-elf-objcopy -Oihex -S prog.elf prog.hex
riscv64-unknown-elf-objcopy -Obinary -S prog.elf prog.bin
