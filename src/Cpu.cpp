/*
 * GB - CPU instruction execution and disassembler
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

#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <utility>
#include <string>
#include "Cpu.h"

using namespace std;

/**< Opcode types emumeration */
enum opcode_type {
    MOV, STS, RPI, LDX, LDS, LDN, LDC, ABS, IMD,
    OPR, BIT, SHF, STK, RST, ROP, IMM, INX, IMS,
    LDD, JMP, CCA, PCHL
};

const string reg_pairs[] = {
    "BC", "BC", "DE", "DE", "HL", "HL", "SP", "AF"
};

const string reg_names[] = {
    "B", "C", "D", "E", "H", "L", "(HL)", "A"
};

const string cc_names[] = {
    "NZ", "Z", "NC", "C"
};

#define ZF(v)  ((!v) << 7)
#define CF(v)  (v << 4)

/**
 * @brief Process add operations.
 *
 * Process add operations, and update the flags.
 * All additions always put their result in A.
 *
 * @param v  value to operate on.
 * @memberof Cpu
 */
inline void Cpu::op_add(uint8_t v)
{
    uint8_t   a = fetch_reg<A>();
    uint8_t   t = a + v;
    uint8_t   c = (a & v) | ((a ^ v) & ~t);

    F = ZF(t) | ((c << 2) & HCAR) | ((c >> 3) & CARRY);
    set_reg<A>(t);
}

/**
 * @brief Process add with carry operations.
 *
 * Process add with carry operations, and update the flags.
 * All additions always put their result in A.
 *
 * @param v  value to operate on.
 * @memberof Cpu
 */
inline void Cpu::op_adc(uint8_t v)
{
    uint8_t   a = fetch_reg<A>();
    uint8_t   c = (F & CARRY) != 0;
    uint8_t   t = a + v + c;

    c = (a & v) | ((a ^ v) & ~t);
    F = ZF(t) | ((c << 2) & HCAR) | ((c >> 3) & CARRY);
    set_reg<A>(t);
}

/**
 * @brief Process subtract operations.
 *
 * Process subtract operations, and update the flags.
 * All subtractions always put their result in A.
 *
 * @param v  value to operate on.
 * @memberof Cpu
 */
inline void Cpu::op_sub(uint8_t v)
{
    uint8_t   a = fetch_reg<A>();
    uint8_t   t;
    uint8_t   c;

    v ^= 0xff;
    t = a + v + 1;
    c = (a & v) | ((a ^ v) & ~t);
    c ^= 0x88;
    F = ZF(t) | ((c << 2) & HCAR) | ((c >> 3) & CARRY) | NFLG;
    set_reg<A>(t);
}

/**
 * @brief Process subtract with borrow operations.
 *
 * Process subtract operations, and update the flags.
 * All subtractions always put their result in A.
 *
 * @param v  value to operate on.
 * @memberof Cpu
 */
inline void Cpu::op_sbc(uint8_t v)
{
    uint8_t   a = fetch_reg<A>();
    uint8_t   t;
    uint8_t   c;

    c = !(F & CARRY);
    v ^= 0xff;
    t = a + v + c;
    c = (a & v) | ((a ^ v) & ~t);
    c ^= 0x88;
    F = ZF(t) | ((c << 2) & HCAR) | ((c >> 3) & CARRY) | NFLG;
    set_reg<A>(t);
}

/**
 * @brief Process logical and operations.
 *
 * Process and operation, and update flags. The
 * carry and negative flag are cleared. Half carry is always set.
 *
 * @param v  value to operate on.
 * @memberof Cpu
 */
inline void Cpu::op_and(uint8_t v)
{
    uint8_t  a = fetch_reg<A>();
    uint8_t  t;

    t = a & v;
    F = ZF(t) | HCAR;
    set_reg<A>(t);
}

/**
 * @brief Process logical xor operations.
 *
 * Process xor operation, and update flags. The
 * carry, half carry and negative flag are cleared.
 *
 * @param v  value to operate on.
 * @memberof Cpu
 */
inline void Cpu::op_xor(uint8_t v)
{
    uint8_t  t;
    uint8_t  a = fetch_reg<A>();

    t = a ^ v;
    F = ZF(t);
    set_reg<A>(t);
}

/**
 * @brief Process logical or operations.
 *
 * Process or operation, and update flags. The
 * carry, half carry and negative flag are cleared.
 *
 * @param v  value to operate on.
 * @memberof Cpu
 */
inline void Cpu::op_or(uint8_t v)
{
    uint8_t  a =fetch_reg<A>();
    uint8_t  t;

    t = a | v;
    F = ZF(t);
    set_reg<A>(t);
}

/**
 * @brief Process compare operations.
 *
 * Process compare, this is a subtract without updating A.
 *
 * @param v  value to operate on.
 * @memberof Cpu
 */
inline void Cpu::op_cp(uint8_t v)
{
    uint8_t   a = fetch_reg<A>();
    uint8_t   t;
    uint8_t   c;

    v ^= 0xff;
    t = a + v + 1;
    c = (a & v) | ((a ^ v) & ~t);
    c ^= 0x88;
    F = ZF(t) | ((c << 2) & HCAR) | ((c >> 3) & CARRY) | NFLG;
}

/**
 * @brief Preform Decimal Adjust instruction.
 *
 * Fixup after addition or subtraction to make number valid BCD.
 * @memberof Cpu
 */
inline void Cpu::op_daa()
{
    uint8_t   a = fetch_reg<A>();
    uint16_t  t = (uint16_t)a;


    if ((F & NFLG) != 0) {   /* Subtract last time */
       if ((F & HCAR) != 0) {
           t = (t - 0x6);
           if ((F & CARRY) == 0) {
               t &= 0xff;
           }
       }
       if ((F & CARRY) != 0) {
           t = (t - 0x60);
       }
    } else {                /* Add last time */
        if ((F & HCAR) != 0 || (t & 0xf) > 9) {
           t += 0x6;
        }
        if ((F & CARRY) != 0 || t > 0x9F) {
           t += 0x60;
        }
    }
    a = (uint8_t)(t & 0xff);
    F &= ~(HCAR|ZERO);
    F |= ZF(a) | (F & NFLG) | (((t & 0x100) != 0) ? CARRY : 0);
    set_reg<A>(a);
}

/**
 * @brief Process increment register instruction.
 *
 * Carry flag is not changed, Half carry is set if carry from bit 3.
 * @memberof Cpu
 */
template <reg_name R>
inline void Cpu::op_inc()
{
    uint8_t r = fetch_reg<R>();
    uint8_t t = r + 1;
    uint8_t hc = ((t & 0xf) == 0) ? HCAR : 0;

    F = ZF(t) | hc | (F & CARRY);
    set_reg<R>(t);
}

/**
 * @brief Process decrement register instruction.
 *
 * Carry flag is not changed, Half carry is set if carry from bit 3.
 * Negative flag set.
 * @memberof Cpu
 */
template <reg_name R>
inline void Cpu::op_dec()
{
    uint8_t   r = fetch_reg<R>();
    uint8_t   t = r + 0xff;
    uint8_t   hc = ((t & 0xf) == 0xf) ? HCAR : 0;

    F = ZF(t) | (hc) | (F & CARRY) | NFLG;
    set_reg<R>(t);
}

/**
 * @brief Process rotate A left instruction.
 *
 * Rotate accumulator left one bit, through carry.
 * Only Carry flag is set, all other flags are cleared.
 * @memberof Cpu
 */
inline void Cpu::op_rla()
{
    uint8_t  c;
    uint8_t  a = fetch_reg<A>();
    c = (a >> 7) & 1;
    a = (a << 1) | ((F & CARRY) != 0);
    F = CF(c);
    set_reg<A>(a);
}

/**
 * @brief Process rotate A left instruction.
 *
 * Rotate accumulator left one bit. Carry set to bit 7.
 * Only Carry flag is set, all other flags are cleared.
 * @memberof Cpu
 */
inline void Cpu::op_rlca()
{
    uint8_t c;
    uint8_t a = fetch_reg<A>();

    c = (a >> 7) & 1;
    a = (a << 1) | (c);
    F = CF(c);
    set_reg<A>(a);
}

/**
 * @brief Process rotate A right instruction.
 *
 * Rotate accumulator right one bit, through carry.
 * Only Carry flag is set, all other flags are cleared.
 * @memberof Cpu
 */
inline void Cpu::op_rra()
{
    uint8_t c;
    uint8_t a = fetch_reg<A>();
    c = (a & 1);
    a >>= 1;
    a |= (F & CARRY) << 3;
    F = CF(c);
    set_reg<A>(a);
}

/**
 * @brief Process rotate A right instruction.
 *
 * Rotate accumulator right one bit. Carry set to bit 0.
 * Only Carry flag is set, all other flags are cleared.
 * @memberof Cpu
 */
inline void Cpu::op_rrca()
{
    uint8_t c;
    uint8_t a = fetch_reg<A>();

    c = a & 1;
    a >>= 1;
    a |= (c << 7);
    F = CF(c);
    set_reg<A>(a);
}

/**
 * @brief Process rotate register left instruction.
 *
 * Rotate register left one bit through carry.
 * Carry and Zero flag are set, Negative and Half carry are cleared.
 *
 * @tparam R Set register to rotate.
 * @memberof Cpu
 */
template <reg_name R>
inline void Cpu::op_rl()
{
    uint8_t c;
    uint8_t r = fetch_reg<R>();

    c = (r >> 7) & 1;
    r = (r << 1) | ((F & CARRY) != 0);
    F = ZF(r) | CF(c);
    set_reg<R>(r);
}

/**
 * @brief Process rotate register left instruction.
 *
 * Rotate register left one bit. Carry set to bit 7.
 * Carry and Zero flag are set, Negative and Half carry are cleared.
 *
 * @tparam R Set register to rotate.
 * @memberof Cpu
 */
template <reg_name R>
inline void Cpu::op_rlc()
{
    uint8_t c;
    uint8_t r = fetch_reg<R>();

    c = r >> 7;
    r = (r << 1) | (c);
    F = ZF(r) | CF(c);
    set_reg<R>(r);
}

/**
 * @brief Process shift register left instruction.
 *
 * Shift register left one bit. Carry set to bit 7. Bit 0 is set to zero.
 * Carry and Zero flag are set, Negative and Half carry are cleared.
 *
 * @tparam R Set register to shift.
 * @memberof Cpu
 */
template <reg_name R>
inline void Cpu::op_sla()
{
    uint8_t c;
    uint8_t r = fetch_reg<R>();

    c = r >> 7;
    r = (r << 1);
    F = ZF(r) | CF(c);
    set_reg<R>(r);
}

/**
 * @brief Process shift register right arithmetic instruction.
 *
 * Shift register right one bit. Bit 7 remains unchanged. Bit 0 to carry.
 * Carry and Zero flag are set, Negative and Half carry are cleared.
 *
 * @tparam R Set register to shift.
 * @memberof Cpu
 */
template <reg_name R>
inline void Cpu::op_sra()
{
    uint8_t c;
    uint8_t r = fetch_reg<R>();

    c = r & 1;
    r = (r >> 1) | (r & SIGN);
    F = ZF(r) | CF(c);
    set_reg<R>(r);
}

/**
 * @brief Process shift register right logical instruction.
 *
 * Shift register right one bit. Bit 7 cleared. Bit 0 to carry.
 * Carry and Zero flag are set, Negative and Half carry are cleared.
 *
 * @tparam R Set register to shift.
 * @memberof Cpu
 */
template <reg_name R>
inline void Cpu::op_srl()
{
    uint8_t c;
    uint8_t r = fetch_reg<R>();

    c = r & 1;
    r = (r >> 1);
    F = ZF(r) | CF(c);
    set_reg<R>(r);
}

/**
 * @brief Process rotate register right instruction.
 *
 * Rotate register right one bit. Carry set to bit 0.
 * Carry and Zero flag are set, Negative and Half carry are cleared.
 *
 * @tparam R Set register to rotate.
 * @memberof Cpu
 */
template <reg_name R>
inline void Cpu::op_rrc()
{
    uint8_t c;
    uint8_t r = fetch_reg<R>();

    c = r & 1;
    r >>= 1;
    r |= (c << 7);
    F = ZF(r) | CF(c);
    set_reg<R>(r);
}

/**
 * @brief Process rotate register right instruction.
 *
 * Rotate register right one bit, through carry.
 * Carry and Zero flag are set, Negative and Half carry are cleared.
 * @tparam R Set register to rotate.
 * @memberof Cpu
 */
template <reg_name R>
inline void Cpu::op_rr()
{
    uint8_t c;
    uint8_t r = fetch_reg<R>();

    c = r & 1;
    r >>= 1;
    r |= (F & 0x10) << 3;
    F = ZF(r) | CF(c);
    set_reg<R>(r);
}

/**
 * @brief Process swap register instruction.
 *
 * Upper and lower nibbles of register are flipped.
 * Zero flag is set, Negative, carry and Half carry are cleared.
 * @tparam R Set register to swap.
 * @memberof Cpu
 */
template <reg_name R>
inline void Cpu::op_swap()
{
    uint8_t r = fetch_reg<R>();

    r = ((r & 0x0F) << 4) | ((r & 0xF0) >> 4);
    F = ZF(r);
    set_reg<R>(r);
}


/**
 * @brief Process bit test instruction.
 *
 * Set zero flag if bit number set.
 * Zero flag is set if bit is zet, Negative is cleared.
 * Half Carry is set, carry is unchanged.
 * @param mask Mask with bit to test.
 * @tparam R Set register to test.
 * @memberof Cpu
 */
template <reg_name R>
inline void Cpu::op_bit(uint8_t mask)
{
    uint8_t r = fetch_reg<R>();

    r &= mask;
    F = ZF(r) | HCAR | (F & CARRY);
}

/**
 * @brief Process bit set instruction.
 *
 * The requested bit is set.
 * @param mask Mask with bit to set.
 * @tparam R Set register to set bit in.
 * @memberof Cpu
 */
template <reg_name R>
inline void Cpu::op_set(uint8_t mask)
{
    uint8_t r = fetch_reg<R>();

    r |= mask;
    set_reg<R>(r);
}

/**
 * @brief Process bit reset instruction.
 *
 * The requested bit is clear.
 * @param mask Mask with bit to clear.
 * @tparam R Set register to clear bit in.
 * @memberof Cpu
 */
template <reg_name R>
inline void Cpu::op_res(uint8_t mask)
{
    uint8_t r = fetch_reg<R>();

    r &= ~mask;
    set_reg<R>(r);
}

/**
 * @brief Process load immediate instruction.
 *
 * Load next byte into register.
 * @tparam R Set register to load next byte into.
 * @memberof Cpu
 */
template <reg_name R>
inline void Cpu::op_ldimd()
{
    uint8_t data;

    data = fetch();
    set_reg<R>(data);
}

/**
 * @brief Process complement accumulator instruction.
 *
 * Complement accumulator, set Negative and Half carry flag.
 * @memberof Cpu
 */
inline void Cpu::op_cpl()
{
    uint8_t a = fetch_reg<A>();
    a ^= 0xff;
    F |= NFLG|HCAR;
    set_reg<A>(a);
}

/**
 * @brief Process set carry instruction.
 *
 * Set the carry flag. Negative and half carry are cleared.
 * @memberof Cpu
 */
inline void Cpu::op_scf()
{
    F &= ZERO;
    F |= CARRY;
}

/**
 * @brief Process complement carry instruction.
 *
 * Reverse the carry flag. Negative and half carry are cleared.
 * @memberof Cpu
 */
inline void Cpu::op_ccf()
{
    F &= ZERO|CARRY;
    F ^= CARRY;
}

/**
 * @brief Process add sp short immediate instruction.
 *
 * Add next byte as signed number to stack pointer.
 * Carry is set if carry out of bit 7, Half Carry if carry out of bit 3.
 * @memberof Cpu
 */
inline void Cpu::op_addsp()
{
    uint8_t  data = fetch();
    int16_t  v = (int16_t)((int8_t)data);
    uint16_t nsp = sp + v;
    uint16_t c = (sp & v) | ((sp ^ v) & ~nsp);

    mem->internal();
    mem->internal();
    sp = nsp;
    F = ((c & 0x0080) ? CARRY : 0) | ((c & 0x0008) ? HCAR: 0);
}

/**
 * @brief Process load index register instruction.
 *
 * Load the next two bytes into requested index register pair.
 * @tparam RP register pair to load address into.
 * @memberof Cpu
 */
template <reg_pair RP>
inline void Cpu::op_ldix()
{
    uint16_t addr;

    addr = fetch_addr();
    setregpair<RP>(addr);
}

/**
 * @brief Process add index register instruction.
 *
 * The index register is added to the HL register.
 * Carry is set if carry out of bit 15, Half Carry if carry out of bit 11.
 * @tparam RP register pair to add into HL.
 * @memberof Cpu
 */
template <reg_pair RP>
inline void Cpu::op_dad()
{
    uint16_t   r = regpair<HL>();
    uint16_t   v = regpair<RP>();
    uint16_t   nr = r + v;
    uint16_t   c = (r & v) | ((r ^ v) & ~nr);
    setregpair<HL>(nr);
    F &= ~(CARRY|HCAR|NFLG);
    F |= ((c & 0x8000) ? CARRY : 0) | ((c & 0x0800) ? HCAR: 0);
    mem->internal();
}

/**
 * @brief Process increment index register instruction.
 *
 * Index register is incremented.
 * @tparam RP register pair to increment.
 * @memberof Cpu
 */
template <reg_pair RP>
inline void Cpu::op_inc16()
{
    uint16_t addr;

    addr = regpair<RP>();
    setregpair<RP>(addr + 1);
    mem->internal();
}

/**
 * @brief Process decrement index register instruction.
 *
 * Index register is decrement.
 * @tparam RP register pair to decrement.
 * @memberof Cpu
 */
template <reg_pair RP>
inline void Cpu::op_dec16()
{
    uint16_t addr;

    addr = regpair<RP>();
    setregpair<RP>(addr - 1);
    mem->internal();
}

/**
 * @brief Process store stack instruction.
 *
 * Stack pointer is stored in address following the instruction.
 * @memberof Cpu
 */
inline void Cpu::op_stsp()
{
    uint16_t addr = fetch_addr();;
    store_double(regpair<SP>(), addr);
}

/**
 * @brief Process load stack plus offset instruction.
 *
 * Add the next byte sign extended to Stack pointer and stored in HL.
 * Carry is set if carry out of bit 7, Half Carry if carry out of bit 3.
 * @memberof Cpu
 */
inline void Cpu::op_ldhl()
{
    uint8_t  data = fetch();
    uint16_t v = (int16_t)((int8_t)data);
    uint16_t nsp = sp + v;
    uint16_t c = (sp & v) | ((sp ^ v) & ~nsp);
    setregpair<HL>(nsp);
    F = ((c & 0x0080) ? CARRY : 0) | ((c & 0x0008) ? HCAR: 0);
    mem->internal();
}

/**
 * @brief Process store accumulator at index instruction.
 *
 * Store accumulator in memory referenced by index register.
 * @tparam RP register pair of where to store accumulator.
 * @memberof Cpu
 */
template <reg_pair RP>
inline void Cpu::op_stax()
{
    uint16_t addr;

    addr = regpair<RP>();
    mem->write(regs[A], addr);
}

/**
 * @brief Process load accumulator at index instruction.
 *
 * Load accumulator in memory referenced by index register.
 * @tparam RP register pair of where to load accumulator.
 * @memberof Cpu
 */
template <reg_pair RP>
inline void Cpu::op_ldax()
{
    uint16_t addr;

    addr = regpair<RP>();
    mem->read(regs[A], addr);
}

/**
 * @brief Process Load/store accumulator to I/O space.
 *
 * Move accumulator to or from I/O space based on next byte.
 * @tparam o  not zero for read, zero for write.
 * @memberof Cpu
 */
template <int o>
inline void Cpu::op_ldst_ff()
{
    uint8_t  offset = fetch();

    if constexpr (o) {
        mem->read(regs[A], 0xff00 + (uint16_t)offset);
    } else {
        mem->write(regs[A], 0xff00 + (uint16_t)offset);
    }
}

/**
 * @brief Process load/store accumulator to I/O space.
 *
 * Move accumulator to or from  I/O space based on C register.
 * @tparam o  not zero for read, zero for write.
 * @memberof Cpu
 */
template <int o>
inline void Cpu::op_ldst_c()
{
    uint8_t  offset = regs[C];

    if constexpr (o) {
        mem->read(regs[A], 0xff00 + (uint16_t)offset);
    } else {
        mem->write(regs[A], 0xff00 + (uint16_t)offset);
    }
}

/**
 * @brief Process load/store accumulator to HL with increment.
 *
 * Read/Write accumulator from (HL), then increment HL.
 * @tparam o  not zero for read, zero for write.
 * @memberof Cpu
 */
template <int o>
inline void Cpu::op_ldi()
{
    uint16_t addr = regpair<HL>();
    if constexpr (o) {
        mem->read(regs[A], addr);
    } else {
        mem->write(regs[A], addr);
    }
    addr++;
    setregpair<HL>(addr);
}

/**
 * @brief Process load/store accumulator to HL with decrement.
 *
 * Read/Write accumulator from (HL), then decrement HL.
 * @tparam o  not zero for read, zero for write.
 * @memberof Cpu
 */
template <int o>
inline void Cpu::op_ldd()
{
    uint16_t addr = regpair<HL>();
    if constexpr (o) {
        mem->read(regs[A], addr);
    } else {
        mem->write(regs[A], addr);
    }
    addr--;
    setregpair<HL>(addr);
}


/**
 * @brief Process load/store accumulator to absolute address.
 *
 * Move accumulator to or from address in next two bytes.
 * @tparam o  not zero for read, zero for write.
 * @memberof Cpu
 */
template <int o>
inline void Cpu::op_ldst_abs()
{
    uint16_t addr;

    addr = fetch_addr();
    if constexpr (o) {
        mem->read(regs[A], addr);
    } else {
        mem->write(regs[A], addr);
    }
}

/**
 * @brief Pop register pair off stack.
 *
 * Pop given register pair off the stack.
 *
 * @tparam RP Register pair to pop.
 * @memberof Cpu
 */
template <reg_pair RP>
inline void Cpu::op_pop()
{
    uint16_t addr;

    addr = pop();
    setregpair<RP>(addr);
}

/**
 * @brief Push register pair to stack.
 *
 * Push given register pair to the stack.
 *
 * @tparam RP Register pair to push.
 * @memberof Cpu
 */
template <reg_pair RP>
inline void Cpu::op_push()
{
    uint16_t addr;
    addr = regpair<RP>();
    mem->internal();
    push(addr);
}

/**
 * @brief Call subroutine at address of next two bytes.
 *
 * Call the subroutine at the address in the next two bytes.
 * If cond is zero, just fetch next two bytes, otherwise
 * push current PC onto stack and transfer to address.
 *
 * @param cond If non-zero do call, otherwise nothing.
 * @memberof Cpu
 */
inline void Cpu::op_call(uint8_t cond)
{
    uint16_t addr;

    addr = fetch_addr();
    if (cond) {
       mem->internal();
       push(pc);
       pc = addr;
    }
}

/**
 * @brief Jump to address in next two bytes.
 *
 * Jump to the address in the next two bytes. If cond is zero,
 * just fetch next two bytes, otherwise transfer to address.
 *
 * @param cond If non-zero do Jump, otherwise nothing.
 * @memberof Cpu
 */
inline void Cpu::op_jp(uint8_t cond)
{
    uint16_t addr;
    addr = fetch_addr();
    if (cond) {
       pc = addr;
       mem->internal();
    }
}

/**
 * @brief Jump to relative to address in next byte.
 *
 * Jump to the sign extended address. If cond is zero,
 * just fetch next two bytes, otherwise transfer to address.
 *
 * @param cond If non-zero do Jump, otherwise nothing.
 * @memberof Cpu
 */
inline void Cpu::op_jr(uint8_t cond)
{
    uint8_t  data = fetch();
    if (cond) {
       pc += (uint16_t)((int16_t)((int8_t)data));
       mem->internal();
    }
}

/**
 * @brief Return from subroutine conditional.
 *
 * If cond is non-zero, pop the old program counter off the
 * stack and transfer to it.
 *
 * @param cond If non-zero do return, otherwise nothing.
 * @memberof Cpu
 */
inline void Cpu::op_ret(uint8_t cond)
{
    if (cond) {
        mem->internal();
        pc = pop();
    }
    mem->internal();
}

/**
 * @brief Return from subroutine.
 *
 * This is seperate subroutine since the timing of conditional
 * return includes an extra internal state where condition is
 * checked. This does not occur on unconditional returns.
 * @memberof Cpu
 */
inline void Cpu::op_return()
{
    pc = pop();
    mem->internal();
}

/**
 * @brief Return from interrupt.
 *
 * Similar to return, only we enable the interrupts. Also interrupts
 * are held off for one cycle.
 * @memberof Cpu
 */
inline void Cpu::op_reti()
{
    pc = pop();
    ime = true;
    mem->internal();
}

/**
 * @brief Reset the CPU to a fixed location.
 *
 * Push the current PC and transfer to location n * 8.
 * @memberof Cpu
 */
inline void Cpu::op_rst(int n)
{
    mem->internal();
    push(pc);
    pc = (n << 3);
}

/**
 * @brief Store HL into stack.
 *
 * @memberof Cpu
 */
inline void Cpu::op_sphl()
{
    sp = regpair<HL>();
    mem->internal();
}

/**
 * @brief Transfer to contents of HL.
 *
 * @memberof Cpu
 */
inline void Cpu::op_pchl()
{
    pc = regpair<HL>();
}

/**
 * @brief Disable interrupts.
 *
 * Set interrupts as disabled.
 *
 * @memberof Cpu
 */
inline void Cpu::op_di()
{
    ime = false;
    ime_hold = false;
}

/**
 * @brief Enable interrupts.
 *
 * Interrupts are enabled, if this instruction is enabling interrupt the
 * next instruction will be executed.
 * @memberof Cpu
 */
inline void Cpu::op_ei()
{
    ime_hold = !ime;
    ime = true;
}

/**
 * @brief Halt the CPU.
 *
 * Halt CPU until an interrupt condition wakes it up.
 * @memberof Cpu
 */
inline void Cpu::op_halt()
{
    halted = true;
}

/**
 * @brief Stop the CPU.
 *
 * Similar to HALT, but only reset and button press can restart it.
 * Display, Timer and Audio are turned all also. Next byte is ignored.
 * @memberof Cpu
 */
inline void Cpu::op_stop()
{
    uint8_t     data;
    uint8_t     irq = irq_en & irq_flg & 0x1f;
    joy.read_reg(data, 0);
    /* Check if Button press */
    if ((data & 0xf) != 0xf) {
        if (irq != 0) {
           halted = true;
           pc++;
        }
        return;
    }

    /* reset DIV register */
    timer.write_reg(0, 0x4);
    if (irq == 0) {
       pc++;
    }
    /* Check if speed change */
    if (key != NULL && key->sw_speed) {
        mem->switch_speed();
    } else {
        stopped = true;
    }
}

/**
 * @brief Nop.
 *
 * Do nothing.
 * @memberof Cpu
 */
inline void Cpu::op_nop()
{
}

/**
 * @brief Process Interrupt request.
 *
 * Process interrupt request, push the PC on the stack and
 * use IF flags to determine which vector to use.
 */

inline void Cpu::do_irq()
{
    uint16_t addr = 0x40;
    uint8_t  data;

    /* Disable interrupt */
    ime = false;
    halted = false;
    mem->internal();
    mem->internal();
    data = ((pc >> 8) & 0xff);
    sp--;
    mem->write(data, sp);
    uint8_t  irq = irq_en;   /* Save for check later */
    data = pc & 0xff;
    sp--;
    mem->write(data, sp);
    pc = 0;
    /* Scan bits looking for enabled and triggered interrupt */
    for (uint8_t i = 0x01; i != 0x20; i <<= 1, addr += 8) {
        if ((irq & irq_flg & i) != 0) {
            pc = addr;
            irq_flg &= ~i;
            break;
        }
    }
    mem->internal();
}

/*
 * Decode for two byte opcodes.
 *
 * SHF expanded to SHFR with each register.
 * SHFR expanded to opcode + register with op_name<register>()
 *
 * BIT expands to BITX with each bit.
 * BITX expands to BITR with register.
 * BITR expands to opcode + bit + register.
 */
#define BITR(name, base2, bit, reg) case base2 + (bit << 3) + (int)(reg): \
                                        op_##name<reg>(1 << bit); break;
#define BITX(name, base2, bit)  BITR(name, base2, bit, B) \
                                BITR(name, base2, bit, C) \
                                BITR(name, base2, bit, D) \
                                BITR(name, base2, bit, E) \
                                BITR(name, base2, bit, H) \
                                BITR(name, base2, bit, L) \
                                BITR(name, base2, bit, M) \
                                BITR(name, base2, bit, A)
#define BIT(name, base1, base2) BITX(name, base2, 0) \
                                BITX(name, base2, 1) \
                                BITX(name, base2, 2) \
                                BITX(name, base2, 3) \
                                BITX(name, base2, 4) \
                                BITX(name, base2, 5) \
                                BITX(name, base2, 6) \
                                BITX(name, base2, 7)
#define SHFR(name, base2, reg)  case base2+(int)(reg): \
                                          op_##name<reg>(); break;
#define SHF(name, base1, base2)  SHFR(name, base2, B) \
                                 SHFR(name, base2, C) \
                                 SHFR(name, base2, D) \
                                 SHFR(name, base2, E) \
                                 SHFR(name, base2, H) \
                                 SHFR(name, base2, L) \
                                 SHFR(name, base2, M) \
                                 SHFR(name, base2, A)
#define INSN_LD(name, type, base)
#define INSN(name, type, base1, base2)
#define INSN2(name, type, base1, base2) type(name, base1, base2)
void Cpu::second(uint8_t data)
{
    switch(data) {
#include "insn.h"
    }
}

#undef BIT
#undef SHF
#undef SHFR
#undef BITX
#undef BITR
#undef INSN_LD
#undef INSN
#undef INSN2

#define INSN(name, type, base1, base2) type(name, base1, base2)
#define INSN2(name, type, base1, base2)
#define INSN_LD(name, type, base) type(name, base)

/* Set up for main operation decode. */
#define REGR(d,s,b)      case b+((int)d<<3)+(int)s: \
                               data = fetch_reg<s>(); set_reg<d>(data);  break;
#define REGS(s,b) REGR(s,B,b) REGR(s,C,b) REGR(s,D,b) REGR(s,E,b) \
                  REGR(s,H,b) REGR(s,L,b) REGR(s,M,b) REGR(s,A,b)
#define REGM(b) REGR(M,B,b) REGR(M,C,b) REGR(M,D,b) REGR(M,E,b) \
                REGR(M,H,b) REGR(M,L,b) REGR(M,A,b)
#define REG(f, b) REGS(B,b) REGS(C,b) REGS(D,b) REGS(E,b) \
                  REGS(H,b) REGS(L,b) REGM(b)   REGS(A,b)
#define STS(f, b) OPR(f, b, 0)
#define LDSTR(f, bl, bs, r) case bl: op_ld##f<r>(); break; \
                           case bs: op_st##f<r>(); break;
#define LDST(f, bl, bs) case bl: op_ldst_##f<1>(); break; \
                        case bs: op_ldst_##f<0>(); break;
#define RPI(f, b) STKS(ld##f, b)
#define LDN(f, b) LDST(f, b+0020, b)
#define LDC(f, b) LDST(f, b+0020, b)
#define LDS(f, b) OPR(f, b, 0)
#define ABS(f, b) LDST(f, b+0020, b)
#define IMD(f, b) OREG(ld##f, b)
#define LDX(f, b) LDSTR(f, b+0x8, b, BC) LDSTR(f, b+0x18, b+0x10, DE)
#define OPR(f, b1, b2) case b1: op_##f(); break;
#define STKS(f,b)      case b:      op_##f<BC>(); break; \
                       case b+0x10: op_##f<DE>(); break; \
                       case b+0x20: op_##f<HL>(); break; \
                       case b+0x30: op_##f<SP>(); break;

#define STK(f,b1,b2)   case b1:      op_##f<BC>(); break; \
                       case b1+0x10: op_##f<DE>(); break; \
                       case b1+0x20: op_##f<HL>(); break; \
                       case b1+0x30: op_##f<AF>(); break;

#define RSTX(f, b,n)   case b+(n<<3): op_##f(n); break;
#define RST(f,b,b2)    RSTX(f, b,0) RSTX(f, b,1) RSTX(f, b,2) RSTX(f, b,3) \
                       RSTX(f, b,4) RSTX(f, b,5) RSTX(f, b,6) RSTX(f, b,7)
#define OREGX(f,b,r)   case b + (((int)r) << 3): op_##f<r>(); break;
#define OREG(f,b)      OREGX(f,b,B) OREGX(f,b,C) OREGX(f,b,D) OREGX(f,b,E) \
                       OREGX(f,b,H) OREGX(f,b,L) OREGX(f,b,M) OREGX(f,b,A)
#define DEC(f,b1,b2)   OREG(f,b1) \
                       STKS(f##16,b2)
#define OPI(f,b)       case b: data = fetch(); op_##f(data); break;
#define OPS(a,f,b)     case a+(int)b: data = fetch_reg<b>(); op_##f(data); break;
#define ROPR(f,b1,b2)  OPS(b1,f,B) OPS(b1,f,C) OPS(b1,f,D) OPS(b1,f,E) \
                       OPS(b1,f,H) OPS(b1,f,L) OPS(b1,f,M) OPS(b1,f,A) \
                       OPI(f, b1+0106)
#define XOPR(f,b1,b2)  ROPR(f,b1,b2) \
                       STKS(dad, b2) \
                       case 0350: op_addsp(); break;
#define IMM(f, b1, b2) OPR(f, b1, b2)
#define LDD(f, b1, b2) case b1:      op_##f<0>(); break; \
                       case b1+0010: op_##f<1>(); break;
#define CAL(f, b1, b2) case b1: op_##f(1); break; \
                       case b2: op_##f((F & ZERO) == 0); break; \
                       case b2+0x08: op_##f((F & ZERO) != 0); break; \
                       case b2+0x10: op_##f((F & CARRY) == 0); break; \
                       case b2+0x18: op_##f((F & CARRY) != 0); break;
#define JMP(f, b1, b2) CAL(f, b1, b2) \
                       case 0351: op_pchl(); break;
#define JMR(f, b1, b2) CAL(f, b1, b2)
#define RET(f, b1, b2) case b1: op_return(); break; \
                       case b2: op_##f((F & ZERO) == 0); break; \
                       case b2+0x08: op_##f((F & ZERO) != 0); break; \
                       case b2+0x10: op_##f((F & CARRY) == 0); break; \
                       case b2+0x18: op_##f((F & CARRY) != 0); break;

/**
 * @brief execute one instruction.
 *
 * Step the CPU by one instruction. If CPU is not running, let everybody
 * know that a clock has occured. Then return right away. If CPU is in
 * stopped state, preform an internal cycle, and read the Joypad to
 * see if any buttons are pressed, at which point we either enter
 * the halt set and take an interrupt. When halted check if there
 * is an interrupt ready, if so exit halt state.
 *
 * Next we clear the interrupt enable holdoff flag to allow interrupts
 * to occur. If CPU is halted, just do internal cycle. Otherwise fetch
 * next byte and decode it with a switch statement. This will result in
 * fetching registers and possibly calling a routine to handle the opcode.
 * For 0313 (0xCB) the opcode is two bytes, so fetch second and call second()
 * to decode it. Lastly check to see if an interrupt is pending, if
 * so let do_irq() save state and set up for next routine.
 */
void Cpu::step()
{
    if (!running) {
        mem->idle();
        return;
    }

    /* Check if stoped state */
    if (stopped) {
        uint8_t     jdata;
        mem->internal();
        joy.read_reg(jdata, 0);
        /* Check if Button press */
        if ((jdata & 0xf) != 0xf) {
            if ((irq_en & irq_flg & 0x1f) == 0) {
                halted = true;
                pc++;
            }
            stopped = false;
        }
        return;
    }

    /* If halted if any interrupts pending, exit halt state */
    if (halted && (irq_en & irq_flg & 0x1f) != 0) {
        if (ime_hold) {
           pc--;
        }
        halted = false;
    }

    /* Clear interrupt hold flag */
    ime_hold = false;
    if (halted) {
        mem->internal();
    } else {
        /* Decode instruction */
        uint8_t ir = fetch();
        uint8_t data;
        switch(ir) {
#include "insn.h"
        case 0313:
               data = fetch();
               second(data);
               break;
        }
    }

    /* Check if interrupt pending. */
    if (ime && !ime_hold && (irq_en & irq_flg & 0x1f) != 0) {
       do_irq();
    }
}

#undef INSN
#undef INSN2
#undef INSN_LD
#undef REGR
#undef REGS
#undef REGM
#undef REG
#undef STS
#undef LDSTR
#undef LDST
#undef RPI
#undef LDN
#undef LDC
#undef ABS
#undef LDX
#undef OPR
#undef STKS
#undef STK
#undef RSTX
#undef RST
#undef OREGX
#undef OREG
#undef DEC
#undef OPI
#undef OPS
#undef ROPR
#undef XOPR
#undef IMM
#undef LDD
#undef LDS
#undef IMD
#undef CAL
#undef JMP
#undef JMR
#undef RET

string toUpper(string str)
{
    for (auto& c : str) {
        c = static_cast<char>(std::toupper(c));
    }

    return str;
}

#define INSN_LD(name, type, base) type(base)
#define REG(b) { toUpper("ld"), MOV, b, 0300, 1},  /* LD r1,r2 */
#define STS(b) { toUpper("ld"), STS, b, 0377, 3},  /* LD (abs),SP */
#define RPI(b) { toUpper("ld"), RPI, b, 0317, 3},  /* LD rp,abs */
#define LDX(b) { toUpper("ld"), LDX, b, 0347, 1},  /* LD (x),A  x=BC/DE */
#define LDS(b) { toUpper("ld"), LDS, b, 0377, 1},  /* SP,HL */
#define LDN(b) { toUpper("ld"), LDN, b, 0357, 2},  /* LD ($FF00+n),A */
#define LDC(b) { toUpper("ld"), LDC, b, 0357, 1},  /* LD (C),A */
#define ABS(b) { toUpper("ld"), ABS, b, 0357, 1},  /* LD (abs),A */
#define IMD(b) { toUpper("ld"), IMD, b, 0307, 2},  /* LD r,# */
#define OPR(f, b1, b2) { toUpper(#f), OPR, b1, 0377, 1}, /* f */
#define BIT(f, b1, b2) { toUpper(#f), BIT, b2, 0300, 2}, /* f b,r */
#define SHF(f, b1, b2) { toUpper(#f), SHF, b2, 0370, 2}, /* f r */
#define STK(f, b1, b2) { toUpper(#f), STK, b1, 0317, 1}, /* f rp */
#define RST(f, b1, b2) { toUpper(#f), RST, b1, 0307, 1}, /* f b */
#define ROPR(f, b1, b2) { toUpper(#f), ROP, b1, 0370, 1}, /* f a,r */ \
                        { toUpper(#f), IMM, b1+0106, 0377, 2},  /* f a,# */
#define XOPR(f, b1, b2) ROPR(f, b1, 0) \
                        { toUpper(#f), INX, b2, 0317, 1}, /* f hl,rp */ \
                        { toUpper(#f), IMS, 0350, 0377, 2},  /* f sp,# */
#define DEC(f,b1,b2) { toUpper(#f), ROP, b1, 0307, 1}, /* f r */ \
                     STK(f, b2, 0)
#define IMM(f, b1, b2) { toUpper(#f), IMS, b1, 0377, 2},  /* ldhl sp,# */
#define LDD(f, b1, b2) { toUpper(#f), LDD, b1, 0347, 1},  /* (hl),a */
#define CAL(f, b1, b2) { toUpper(#f), JMP, b1, 0377, 3}, \
                       { toUpper(#f), CCA, b2, 0347, 3}, /* 11 000 100 */
#define JMP(f, b1, b2) CAL(f, b1, b2) \
                       { toUpper(#f), CCA, b2, 0347, 3}, /* 11 000 010 */ \
                       { toUpper(#f), PCHL, 0351, 0377, 1},
#define JMR(f, b1, b2) { toUpper(#f), JMP, b1, 0377, 2}, \
                       { toUpper(#f), CCA, b2, 0347, 2}, /* 00 100 000 */
#define RET(f, b1, b2) OPR(f, b1, 0) \
                       { toUpper(#f), CCA, b2, 0347, 1}, /* 11 000 000 */

#define INSN(name, type, base, mod) type(name, base, mod)
#define INSN2(name, type, base, mod) type(name, base, mod)

struct opcode {
    string       name;
    opcode_type  type;
    uint8_t      base;
    uint8_t      mask;
    int          len;
} opcode_map[] = {
#include "insn.h"
    { "", OPR, 0, 0, 0}
};

string Cpu::disassemble(uint8_t ir, uint16_t addr, int &len)
{
    const struct opcode *op;
    stringstream temp;
    bool         two_byte = false;

    len = 1;
    /* If two byte opcode, switch to second opcode */
    if (ir == 0313) {
        two_byte = true;
        ir = 0xff;
    }

    /* Search opcode map for match */
    for(op = opcode_map; op->name != ""; op++) {
        if ((ir & op->mask) == op->base) {
            if (two_byte) {
               if (op->type == BIT || op->type == SHF)
                   break;
            } else {
               if (op->type != BIT && op->type != SHF)
                   break;
            }
        }
    }

    /* If not found dump possible opcode */
    if (op->name == "") {
        temp << hex << internal << setw(2) << setfill('0') << (unsigned int)(ir) << " ";
        temp << hex << internal << setw(4) << setfill('0') << addr;
        return temp.str();
    }


    len = op->len;
    temp << hex << internal << setw(2) << setfill('0') << (unsigned int)(ir) << " ";
    switch(op->type) {
    case BIT:
        temp << op->name << " " << ((ir >> 3) & 07) << "," << reg_names[ir & 07];
        break;
    case SHF:
        temp << op->name << " " << reg_names[ir & 07];
        break;
    case OPR:
        temp << op->name;
        break;
    case MOV:
        temp << op->name << " " << reg_names[(ir >> 3) & 07] <<
             "," << reg_names[(ir & 07)];
        break;
    case STS:
        temp << op->name;
        temp << " (" << std::hex << addr;
        temp << "),SP";
        break;
    case RPI:
        temp << op->name << " " << reg_pairs[(ir >> 3) & 06] << ","
                     << std::hex << addr;
        break;
    case LDX:
        if (ir & 0x08) {
           temp << op->name << " A,(" << reg_pairs[(ir >> 3) & 06] << ")";
        } else {
           temp << op->name << " (" << reg_pairs[(ir >> 3) & 06] << "),A";
        }
        break;
    case LDN:
        temp << op->name << " ";
        addr &= 0xff;
        addr |= 0xff00;
        if (ir & 0x10) {
           temp << "A,($" << std::hex << addr << ")";
        } else {
           temp << "($" << std::hex << addr << "),A";
        }
        break;
    case LDC:
        if (ir & 0x10) {
             temp << op->name << " A,($FF00+C)";
        } else {
             temp << op->name << " ($FF00+C),A";
        }
        break;
    case ABS:
        if (ir & 0x10) {
           temp << op->name << " A,($" << std::hex << addr << ")";
        } else {
           temp << op->name << " ($" << std::hex << addr << "),A";
        }
        break;
    case LDD:
        if (ir & 0x8) {
           temp << op->name << " A,(HL)";
        } else {
           temp << op->name << " (HL),A";
        }
        break;
    case IMD:
        temp << op->name << " " << reg_names[(ir >> 3) & 07] <<
                        ",$" << std::hex << (addr & 0xff);
        break;
    case STK:
        if ((ir & 0xc0) != 0) {
            temp << op->name << " " << reg_pairs[(((ir >> 3) & 06) | 1)];
        } else {
            temp << op->name << " " << reg_pairs[((ir >> 3) & 06)];
        }
        break;
    case LDS:
        temp << op->name << " " << reg_pairs[((ir >> 3) & 06)] << ",HL";
        break;
    case RST:
        temp << op->name << " " << std::hex << ((ir >> 3) & 07);
        break;
    case IMM:
        temp << op->name << " A,$" << std::hex << (addr & 0xff);
        break;
    case IMS:
        temp << op->name << " SP,$" << std::hex << (addr & 0xff);
        break;
    case INX:
        temp << op->name << " HL," << reg_pairs[((ir >> 3) & 06)];
        break;
    case ROP:
        if ((ir & 0300) == 0200) {
            temp << op->name << " A," << reg_names[(ir) & 07];
        } else {
            temp << op->name << " " << reg_names[(ir>>3) & 07];
        }
        break;
    case JMP:
        temp << op->name << " " << std::hex << addr;
        break;
    case CCA:
        if (op->len == 3) {
            temp << op->name << " " << cc_names[(ir >> 3) & 3] <<
                        "," << std::hex << addr;
        } else if (op->len == 2) {
            temp << op->name << " " << cc_names[(ir >> 3) & 3] <<
                        "," << std::hex << (addr & 0xff);
        } else {
            temp << op->name << " " << cc_names[(ir >> 3) & 3];
        }
        break;
    case PCHL:
        temp << op->name << " (HL)";
        break;
    }
    return temp.str();
}

string Cpu::dumpregs(uint8_t regs[8])
{
    int i;
    stringstream temp;

    for(i = 0; i < 8; i++) {
        if (i == M)
            continue;
        temp << reg_names[i] << "=";
        temp << hex << internal << setw(2) << setfill('0') << (unsigned int)(regs[i]) << " ";
    }
    return temp.str();
}

void Cpu::trace()
{
    uint16_t  addr;
    uint8_t   ir;
    uint8_t   t;
    int       len;
    uint8_t   div;

    timer.read_reg(div, 0);
    mem->read_nocycle(ir, pc);
    mem->read_nocycle(t, pc+1);
    addr = t;
    mem->read_nocycle(t, pc+2);
    addr |= (t << 8);
    cout << dumpregs(regs) << "SP=" << hex << internal << setfill('0') << setw(4) << sp << " ";
    cout << hex << internal << setfill('0') << setw(4) << pc << " ";
    cout << "Div=" << hex << setfill('0') << setw(2) << (unsigned int)div << " ";
    cout << "F=" << hex << internal << setfill('0') << setw(2) << (unsigned int)(F) << " ";
    cout << "I=" << hex << internal << (unsigned int)(ime) << " ";
    cout << "IF=" << hex << internal << (unsigned int)(irq_flg) << " ";
    cout << disassemble(ir, addr, len) << endl;
}

