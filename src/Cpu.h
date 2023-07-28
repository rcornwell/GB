/*
 * GB - CPU class header file.
 *
 * Author:      Richard Cornwell (rich@sky-visions.com)
 * Copyright 2023, Richard Cornwell
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include <string>
#include <sstream>
#include <stdint.h>
#include "Memory.h"
#include "IO.h"
#include "Cartridge.h"
#include "Joypad.h"
#include "Timer.h"
#include "Ppu.h"
#include "Apu.h"
#include "Serial.h"


#define SIGN  0x80
#define ZERO  0x80
#define NFLG  0x40
#define HCAR  0x20
#define CARRY 0x10

enum reg_name {
    B, C, D, E, H, L, M, A
};

enum reg_pair {
    BC, DE, HL, SP, AF
};

/**
 * @brief Game Boy CPU emulation.
 *
 * Emulates the Game Boy CPU. This object requires that a Cartridge
 * be attached when it is created. Also to isolate SDL operations these
 * have been moved to the Screen object.
 *
 * At initialization the CPU first creates a Memory object and RAM
 * Object. The Memory object is hooked up with the Timer and PPU object
 * so that CPU cycles can be passed to these objects. Next the Cartridge
 * is attached to the Memory object. The Timer object is attached to the Apu
 * to allow for samples to be properly timed. Next all devices are hooked
 * to the IO object, and these are then registered with interrupt variables.
 *
 * step() is called to preform each instruction.
 */
class Cpu
{
public:
    /**
     * @brief Initialize a CPU object.
     *
     * Initialize the CPU by creating a Memory object and a RAM object.
     * These are used to access memory and represent Working RAM. The
     * RAM, Cartridge, Ppu are given pointers to Memory so they can
     * enable and disable regions as needed.
     *
     * Timer is linked to APU so that it can send sample intervals.
     * The devices are hooked up to the I/O object, and finally the
     * I/O device is given pointers to the interrupt variables. The
     * I/O object will pass this on to the devices.
     */
    explicit Cpu(Cartridge *_cart) : cart(_cart)
    {
        /* Create memory and ram objects */
        mem = new Memory(&timer, &ppu, &apu, &ser);
        ram = new RAM(8192);
        io = new IO_Space(&irq_en, &irq_flg);
        /* These need to be added first since 0xe000 overlaps I/O and video space */
        mem->add_slice(ram, 0xc000);
        mem->add_slice(ram, 0xe000);
        /* Map I/O space into memory */
        mem->add_slice(io,  0xff00);
        /* Connect cartridge to memory object */
        cart->set_mem(mem);
        cart_dev.set_cart(cart);
        /* PPU needs to manage memory */
        ppu.set_mem(mem);
        /* System needs to know about joypad to send button presses to it. */
        set_joypad(&joy);
        /* Add in devices */
        io->add_device(&timer);
        io->add_device(&ppu);
        io->add_device(&joy);
        io->add_device(&apu);
        io->add_device(&ser);
        io->add_device(&cart_dev);
        /* Timer needs to send events to APU */
        timer.set_apu(&apu);

        running = false;
        F = ZERO;
        sp = 0;
        pc = 0;
        ime = false;
        ime_hold = false;
        halted = false;
        irq_en = 0x00;
        irq_flg = 0x00;
        for (int i = 0; i < 8; i++) {
            regs[i] = 0;
        }
    }

    Cpu(Cpu &) = delete;

    Cpu& operator=(Cpu &) = delete;

    virtual ~Cpu()
    {
        delete io;
        delete ram;
        delete mem;
    }

    uint8_t   regs[8];   /**< Cpu registers B,C,D,E,H,L,M,A */
    uint8_t   F;         /**< Cpu flags */
    uint16_t  sp;        /**< Stack pointer */
    uint16_t  pc;        /**< Program counter */

    bool      ime;       /**< Interrupt enable flag */
    bool      ime_hold;  /**< Hold interrupts off for one cycle */


    uint8_t   irq_en;    /**< Interrupt enable register */
    uint8_t   irq_flg;   /**< Interrupt flags register */
    bool      halted;    /**< Cpu is halted and waiting for interrupt */
    bool      running;   /**< Cpu is running */

    Cartridge  *cart;    /**< Cartridge with game */
    Cartridge_Device cart_dev; /**< Cartridge device, disable ROM */
    Memory    *mem;      /**< Memory object */
    RAM       *ram;      /**< Working RAM */
    IO_Space  *io;       /**< I/O Space */
    Timer     timer;     /**< Timer device */
    Ppu       ppu;       /**< Graphics processing unit */
    Apu       apu;       /**< Audio processing unit */
    Joypad    joy;       /**< Game pad buttons */
    Serial    ser;       /**< Serial data link controller */

    /**
     * @brief Return number of cycles executed.
     *
     * @return cycles.
     */
    uint64_t  get_cycles()
    {
        return mem->get_cycles();
    }

    /**
     * @brief Tell memory how many cycles we should have run.
     *
     * @param max_cycles Maximum number of cycles we should have run.
     */
    void reset_cycles(int max_cycles) {
        mem->reset_cycles(max_cycles);
    }

    /**
     * @brief Fetches the contents of memory location pointed to by HL.
     * @return Value of memory location.
     */
    inline uint8_t fetch_mem()
    {
        uint8_t t;
        mem->read(t, regpair<HL>());
        return t;
    }

    /**
     * @brief Fetch the register specified by the parameter R.
     *
     * This function is templated to allow for inline generation of the correct
     * function based on definitions. The code should be expanded to just fetch
     * the value or the contects of memory.
     *
     * @tparam R register to fetch.
     * @return Value of the register/memory.
     */
    template <reg_name R>
    constexpr inline uint8_t fetch_reg()
    {
        uint8_t t;
        if constexpr (R != M)
            t = regs[(int)R];
        else
            t = fetch_mem();
        return t;
    }

    /**
     * @brief Sets the register pointed to by the parameter R.
     * @param value Value to set register to.
     * @tparam R register to Set the value.
     * @return Void.
     */
    template <reg_name R>
    constexpr inline void set_reg(uint8_t value)
    {
        if constexpr (R != M)
            regs[(int)R] = value;
        else
            mem->write(value, regpair<HL>());
    }

    /**
     * @brief Fetch the register pair pointed to by the parameter RP.
     *
     * The register is returned as a 16 bit value.
     * @tparam RP register pair to fetch.
     * @return Value of register pair.
     */
    template <reg_pair RP>
    inline uint16_t regpair()
    {
        uint16_t t;
        if constexpr (RP == BC) {
            t = ((uint16_t)(regs[B]) << 8) |
                ((uint16_t)(regs[C]));
        } else if constexpr (RP == DE) {
            t = ((uint16_t)(regs[D]) << 8) |
                ((uint16_t)(regs[E]));
        } else if constexpr (RP == HL) {
            t = ((uint16_t)(regs[H]) << 8) |
                ((uint16_t)(regs[L]));
        } else if constexpr (RP == SP) {
            t = sp;
        } else if constexpr (RP == AF) {
            t = (uint16_t)(F) |
                ((uint16_t)(regs[A])<<8);
        } else {
            t = 0;
        }
        return t;
    }

    /**
     * @brief Sets the register pair specified by the parameter RP
     * @tparam RP register pair to set.
     * @param value Value to set register pair to.
     * @return Void
     */
    template <int RP>
    constexpr inline void setregpair(uint16_t value)
    {
        if constexpr (RP == BC) {
            regs[B] = (value >> 8) & 0xff;
            regs[C] = value & 0xff;
        } else if constexpr (RP == DE) {
            regs[D] = (value >> 8) & 0xff;
            regs[E] = value & 0xff;
        } else if constexpr (RP == HL) {
            regs[H] = (value >> 8) & 0xff;
            regs[L] = value & 0xff;
        } else if constexpr (RP == SP) {
            sp = value;
        } else if constexpr (RP == AF) {
            F = value & 0xf0;
            regs[A] = (value >> 8) & 0xff;
        }
    }

    /**
     * @brief Fetch the next byte pointed to by the program counter.
     *
     * After the fetch the program counter is incremented by 1.
     *
     * @return Next byte pointed to by program counter.
     */
    inline uint8_t fetch()
    {
        uint8_t temp;

        mem->read(temp, pc);
        pc ++;
        pc &= 0xffff;
        return temp;
    }

    /**
     * @brief Return the address at the program counter.
     *
     * The next two bytes of program memory are read and converted to a
     * 16 bit value.
     *
     * @return Address value of next two bytes.
     */
    inline uint16_t fetch_addr()
    {
        uint16_t value;
        uint8_t  temp;
        mem->read(temp, pc);
        pc ++;
        pc &= 0xffff;
        value = temp;
        mem->read(temp, pc);
        pc++;
        pc &= 0xffff;
        value |= ((uint16_t)temp) << 8;
        return value;
    }

    /**
     * @brief Returns the address word from the two bytes at addr.
     * @param addr Address to fetch next two bytes from.
     * @return Next two bytes as address.
     */
    inline uint16_t fetch_double(uint16_t addr)
    {
        uint16_t value;
        uint8_t  temp = 0;

        mem->read(temp, addr);
        addr ++;
        addr &= 0xffff;
        value = temp;
        mem->read(temp, addr);
        value |= ((uint16_t)temp) << 8;
        return value;
    }

    /**
     * @brief Save the address (value) into the two bytes pointed to by
     * addr.
     * @param value Value to save.
     * @param addr Location to save value.
     */
    inline void store_double(uint16_t value, uint16_t addr)
    {
        uint8_t temp = (value & 0xff);
        mem->write(temp, addr);
        addr ++;
        addr &= 0xffff;
        temp = (value >> 8) & 0xff;
        mem->write(temp, addr);
    }

    /**
     * @brief Push value as two byte value onto stack.
     * @param value to push
     */
    inline void push(uint16_t value)
    {
        uint8_t temp = ((value >> 8) & 0xff);
        sp--;
        mem->write(temp, sp);
        temp = value & 0xff;
        sp--;
        mem->write(temp, sp);
    }

    /**
     * @brief Pop next two byte value off stack.
     * @return Value popped.
     */
    inline uint16_t pop()
    {
        uint16_t  value;
        value = fetch_double(sp);
        sp += 2;
        return value;
    }

    void do_irq();

    /**
     * @brief Process second byte of two byte opcode.
     *
     * @param op Opcode to evaluate.
     */
    void second(uint8_t op);

    void step();

    /**
     * @brief Print out current instruction.
     */
    void trace();

    /**
     * @brief Set CPU to running and not halted.
     */
    void run()
    {
        running = true;
    };

    /**
     * @brief Disassembler current instruction.
     */
    std::string disassemble(uint8_t ir, uint16_t addr, int &len);

    /**
     * @brief Dump CPU registers.
     */
    std::string dumpregs(uint8_t regs[8]);

private:
    /**
     * @brief Convert INSN macro into definitions for instructions.
     *
     * INSN defines an instruction name, the type of the instruction
     * and the base of the opcode. All opcodes are defined as op_name().
     * Depending on the function some will expand based on the register
     * or the register pair of there argument.
     **/

#define OPR(name)    void op_##name();
#define DEC(name)    template <reg_name R>void op_##name(); \
                     template <reg_pair RP>void op_##name();
#define BIT(name)    template <reg_name R>void op_##name(uint8_t bit);
#define JMP(name)    CAL(name) \
                     void op_pchl();
#define CAL(name)    void op_##name(uint8_t cond);
#define JMR(name)    CAL(name)
#define RET(name)    CAL(name) void op_return();
#define LDD(name)    template <int o>void op_##name();
#define IMM(name)    OPR(name)
#define SHF(name)    template <reg_name R>void op_##name();
#define OP2(name)    OPR(name)
#define STK(name)    template<reg_pair RP>void op_##name();
#define ROPR(name)   void op_##name(uint8_t v);
#define XOPR(name)   ROPR(name) \
                     STK(dad) \
                     void op_addsp();
#define RST(name)    void op_##name(int n);
#define REG(name)
#define STS(name)    void op_stsp();
#define RPI(name)    STK(ld##name)
#define LDX(name)    STK(ld##name) \
                     STK(st##name)
#define LDN(name)    template <int o>void op_ldst_##name();
#define LDS(name)    void op_sphl();
#define LDC(name)    LDN(name)
#define ABS(name)    LDN(name)
#define IMD(name)    template<reg_name R>void op_ld##name();

#define INSN_LD(name, type, base) type(name)
#define INSN(name, type, base, base2) type(name)
#define INSN2(name, type, base, base2) type(name)

/*! \include insn.h
 */
#include "insn.h"

#undef OPR
#undef ROPR
#undef XOPR
#undef BIT
#undef DEC
#undef CAL
#undef JMP
#undef JMR
#undef RET
#undef LDD
#undef IMM
#undef SHF
#undef OP2
#undef STK
#undef RST
#undef REG
#undef STS
#undef RPI
#undef LDX
#undef LDN
#undef LDS
#undef LDC
#undef ABS
#undef IMD
#undef INSN_LD
#undef INSN
#undef INSN2
};
