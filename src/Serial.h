/*
 * GB - Serial Data Link
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

#include <cstdint>


/**
 * @brief Serial data link
 *
 */
class Serial {
protected:
     uint8_t     *_irq_flg;            /**< Interrupt flag pointer */
     uint8_t      _buffer;             /**< Serial buffer */
     uint8_t      _out;                /**< Character output */
     uint8_t      _in;                 /**< Next character to read */
     int          _count;              /**< Current bit count */
     bool         _xfer;               /**< Tranfer in progress */
     bool         _clock;              /**< Clock internal/external */
     int          _inter;              /**< Interval timer */

public:

     Serial () : _irq_flg(NULL), _buffer(0), _out(0), _in(0xff), _count(0), 
                 _xfer(false), _clock(false), _inter(2) { };

     /**
      * @brief Connect to interrupt register
      *
      * Set pointer to where interrupts need to be updated.
      *
      * @param irq_flg Pointer to interrupt register in CPU.
      */
     virtual void set_irq(uint8_t *irq_flg) {
         _irq_flg = irq_flg;
     }

     /**
      * @brief Post interrupt flag.
      *
      * When there is a change in button state, generate an interrupt.
      * @param value mask of bit to set..
      */
     virtual void post_irq(uint8_t value) {
         *_irq_flg |= value;
     }

     /**
       * @brief Process a single cycle.
       *
       * Serial link operates at a 8192Hz clock rate, so transfer one bit
       * every 128 clock times.
       */
     void cycle() {
        if (++_inter == 128) {
            _inter = 0;
            if (_xfer) {
                uint8_t    bit = (_buffer & 0x80) != 0;
                
// printf("Serial %02x< %02x <%02x %d\n", _out, _buffer, _out, _count);
                _buffer <<= 1;
                if (_in & 0x80) {
                   _buffer |= 1;
                }
                _out <<= 1;
                if (bit) {
                   _out |= 1;
                }
                if (++_count == 8) {
                   /* Select data regiser */
                   std::cerr << (char)_out;
                   _count = 0;
                   _xfer = false;
                   _in = 0xff;
                   post_irq(0x8);
                }
            }
         }
     }


     /**
      * @brief Read Serial data.
      *
      * Use outbits to decide which half of the register to output.
      * @param[out] data Data read from serial link.
      * @param[in] addr Address to read from.
      */
     virtual void read(uint8_t &data, uint16_t addr) {
         if (addr & 1) {
             /* Select data regiser */
             data = _buffer;
         } else {
             /* Select control register */
             data = 0x7e;
             if (_xfer) {
                data |= 0x80;
             }
             data |= _clock;
         }
     }

     /**
      * @brief Write Serial data.
      *
      * Set outbits indicated which row to select. 
      * @param[in] data Data to write to serial link.
      * @param[in] addr Address to write to.
      */
     virtual void write(uint8_t data, uint16_t addr) {
         if (addr & 1) {
             _buffer = data;
// printf("Write serial %02x\n", _buffer);
         } else {
             /* Select control register */
             _clock = data & 1;
// printf("Write cont %02x\n", data);
             if (data & 0x80) {
                 _count = 0;
                 _xfer = true;
// printf("Start serial %02x\n", _buffer);
             }
         }
     }

};

