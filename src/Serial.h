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
#include "Device.h"


/**
 * @brief Serial data link
 *
 */
class Serial : public Device {
protected:
     uint8_t      _buffer;             /**< Serial buffer */
     uint8_t      _out;                /**< Character output */
     uint8_t      _in;                 /**< Next character to read */
     int          _count;              /**< Current bit count */
     bool         _xfer;               /**< Tranfer in progress */
     bool         _clock;              /**< Clock internal/external */
     int          _inter;              /**< Interval timer */

public:

     Serial () : _buffer(0), _out(0), _in(0xff), _count(0),
                 _xfer(false), _clock(false), _inter(2) { };

     /**
      * @brief Address of Serial unit.
      *
      * @return base address of device.
      */
     virtual uint8_t reg_base() const override {
         return 0x1;
     }

     /**
      * @brief Number of registers Serial unit has.
      *
      * @return number of registers.
      */
     virtual int reg_size() const override {
         return 2;
     }

     /**
       * @brief Process a single cycle.
       *
       * Serial link operates at a 8192Hz clock rate, so transfer one bit
       * every 128 clock times.
       */
     virtual void cycle() override {
        if (++_inter == 128) {
            _inter = 0;
            if (_xfer && _clock) {
                uint8_t    bit = (_buffer & 0x80) != 0;

if (trace_flag) {printf("Serial %02x< %02x <%02x %d\n", _out, _buffer, _out, _count);}
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
                   post_irq(SERIAL_IRQ);
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
     virtual void read_reg(uint8_t &data, uint16_t addr) const override {
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
     virtual void write_reg(uint8_t data, uint16_t addr) override {
         if (addr & 1) {
             _buffer = data;
if (trace_flag) printf("Write serial %02x\n", _buffer);
         } else {
             /* Select control register */
             _clock = data & 1;
// printf("Write cont %02x\n", data);
             if (data & 0x80) {
                 _count = 0;
                 _xfer = true;
if (trace_flag) printf("Start serial %02x\n", _buffer);
             }
         }
     }

};

