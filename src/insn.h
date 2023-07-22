/*
 * GB - Instruction data.
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


/*
 *
 *  ROPR      a,r    a,#
 *           op+r   op+0106 d
 *
 *  XOPR      a,r    a,#        hl,rp    sp,#
 *           op+r   op+0106 d   op2+rp   350
 *
 *  BIT       b,r
 *         op op2+r
 *
 *  JMP       abs       cc,abs    jp  (hl)
 *           op i h  op2+cc l h   351
 *
 *  JMC       abs       cc,abs
 *           op i h  op2+cc l h
 *
 *  OPR
 *            op
 *
 *  DEC       r       r
 *          op+r      op2+rp
 *
 *  LDD      (hl),a    a,(hl)
 *           op      op+010
 *
 *  IMM       SP,n
 *            op l h
 *
 *  JMR       rel    cc,rel
 *           op r   op2+cc r
 *
 *  SHF       r
 *         op op2+r
 *
 *  STK      rp
 *           op+rp
 *
 *  OP2
 *           op  op2
 *
 *
 *  cc = 000 NZ, 010 Z, 020 NC, 030 C
 */


INSN(and, ROPR,0240,0)              /* a,r a,# */
INSN(add, XOPR,0200,0011)         /* a,r a,# hl,rp sp,# */
INSN(adc, ROPR,0210,0)
INSN(call,CAL,0315,0304)         /* abs cc,abs */
INSN(ccf, OPR,0077,0)
INSN(cp,  ROPR,0270,0)               /* a,r a,# */
INSN(cpl, OPR,0057,0)
INSN(daa, OPR,0047,0)
INSN(dec, DEC,0005,0013)          /* rp r */
INSN(di,  OPR,0363,0)
INSN(ei,  OPR,0373,0)
INSN(halt,OPR,0166,0)
INSN(ldi, LDD,0042,0)             /* (hl),a  a,(hl) */
INSN(ldd, LDD,0062,0)             /* (hl),a  a,(hl) */
INSN(ldhl,IMM,0370,0)            /* SP,n */
INSN(jr,  JMR,0030,0040)         /* rel cc,rel */
INSN(jp,  JMP,0303,0302)         /* abs cc,abs */
INSN(inc, DEC,0004,0003)        /* rp r */
INSN(nop, OPR,0000,0)
INSN(or,  ROPR,0260,0)             /* a,r a,# */
INSN(ret, RET,0311,0300)        /* none  cc */
INSN(reti,OPR,0331,0)
INSN2(rl,  SHF,0313,0020)              /* r */
INSN(rla, OPR,0027,0)
INSN(rlca,OPR,0007,0)
INSN(rra, OPR,0037,0)
INSN(rrca,OPR,0017,0)
INSN(rst, RST,0307,0)             /* n * 8 */
INSN(pop, STK,0301,0)             /* rp */
INSN(push,STK,0305,0)            /* rp */
INSN(sbc, ROPR,0230,0)            /* a,r a,# */
INSN(sub, ROPR,0220,0)            /* a,r a,# */
INSN(scf, OPR,0067,0)
INSN(stop,OP2,0020,0)
INSN(xor, ROPR,0250,0)            /* a,r a,# */

INSN2(bit, BIT,0313,0100)           /* b,r */
INSN2(res, BIT,0313,0200)             /* b,r */
INSN2(rlc, SHF,0313,0000)              /* r */
INSN2(rr,  SHF,0313,0030)              /* r */
INSN2(rrc, SHF,0313,0010)             /* r */
INSN2(swap,SHF,0313,0060)            /* r */
INSN2(set, BIT,0313,0300)             /* b,r */
INSN2(sla, SHF,0313,0040)             /* r */
INSN2(sra, SHF,0313,0050)             /* r */
INSN2(srl, SHF,0313,0070)             /* r */

/*
 * LD instructions:
 *
 * REG   rr1,rr2
 *        op + (rr1 << 3) + rr2    rr 6 = (HL)
 *
 * STS   (abs),SP
 *        op l h
 *
 * RPI    rp,abs   rp = BC,DE,HL,SP
 *       op + (rp << 4) l h
 *
 * LDX    (x),A         A,(x)   x=BC,DE
 *       op+(x<<4)     op+8+(x<<4)
 *
 * LDN    ($FF00+n),A   A,($FF00+n)
 *        op n         op+0x10  n
 *
 * LDS    sp,hl
 *        op
 *
 * LDC    (c),A  A,(c)
 *       op      op+0x10
 *
 * ABS    (abs),A   A,(abs)
 *       op l h      op+0x10 l h
 *
 * IMD    r,#
 *       op + (r << 3)
 *
 */

INSN_LD(reg,  REG,0100)               /* rr1,rr2 */
INSN_LD(stsp, STS,0010)               /* (abs),SP */
INSN_LD(ix,   RPI,0001)               /* rp,abs */
INSN_LD(ax,   LDX,0002)               /* (x),A A,(x) */
INSN_LD(ff,   LDN,0340)               /* ($FF00+n),A A,($FF00+n) */
INSN_LD(sphl, LDS,0371)               /* sp,hl */
INSN_LD(c,    LDC,0342)               /* (c),a a,(c) */
INSN_LD(abs,  ABS,0352)               /* (abs),A A,(abs) */
INSN_LD(imd,  IMD,0006)               /* r,# */
