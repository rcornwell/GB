# Game Boy Emulator

This is a sample C++ emulator for a Game Boy game console. It is written to be simple yet cycle accurate. The Emulator does not provide many features.

## Overview

The two main classes for this emulator is Cpu and Memory. Cpu class handles the fetch, decode and execute of instructions. It calls the Memory object read and write values out of simulated memory. The Memory class divides memory into 256 256 byte objects call Slice. Memory also tracks CPU cycles by calling various devices cycle() function each CPU cycle. Memory also handles DMA transfers.

On initialization all slices of memory are pointed at the Empty object. This object will always respond 0xff to any read and ignores all writes. This behavior is used to disable OAM and VRAM access during video display. This is also used to switch the Boot ROM in and out.

## CPU

The CPU processes instructions. Each call to step() executes one instruction or processes an interrupt. The CPU class also include the device objects. As part of the initialization process for the CPU class Memory space is created along with the working RAM. Then all devices that have memory space are given to the Memory object and all devices are given to IO which manages IO Space and internal RAM. All instructions that the CPU knows are listed in the insn.h file. They use a couple macros INSN, INSN2, INSN_LD. INSN defines most of the instructions, arguments are mnemonic, class, primary binary, secondary binary. For INSN2 they are escape opcode and base opcode. INSN_LD is used for all LD class instructions. By use of macro expansion these can be used to generate function definitions for class, case statements for instruction decode or to generate a table for opcode tracing.

The CPU step() function checks to see if interrupts are enabled and if there are any pending first. If so the current PC is pushed and the next PC is generated. If a HALT instruction is running a check if any interrupt are triggered to leave the halt state. If not halted the byte at PC is read and a switch is preformed to based on the instruction. These result in generating a call to op_name() with the appropriate values as needed.

## Memory

The Memory class holds an array of slices which it chooses to access based on the address accessed. The complete address is handed to the object so it must be masked off. add_slice() is called to add areas into memory. free_slice() will point the range back to empty space to disable access.

## Detailed Documenation

. @subpage basics.md  
. @subpage timer.md  
