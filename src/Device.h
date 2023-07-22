/*
 * GB - Generic interface to devices.
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

#include "Memory.h"

/**
 * @brief Generic device object.
 *
 * Device objects need to be able to post interrupts. And need to 
 * have registers mapped into I/O space.
 *
 * Objects should return the base address in I/O space for there
 * device and the size of registers they need. I/O space will
 * use these values when the device is added to I/O space to fill
 * in pointers to device.
 *
 * This is an Abstract class and can't be created.
 */
class Device {
protected:
     uint8_t     *_irq_flg;

public:
     Device () : _irq_flg(NULL) { };

     virtual void set_irq(uint8_t *irq_flg) {
         _irq_flg = irq_flg;
     }

     virtual void post_irq(uint8_t value) {
         if (_irq_flg) {
            *_irq_flg |= value;
         }
     }

     virtual void read_reg(uint8_t &data, uint16_t addr) = 0;

     virtual void write_reg(uint8_t data, uint16_t addr) = 0;

     virtual void cycle() = 0;

     virtual uint8_t reg_base() = 0;

     virtual int reg_size() = 0;
};

class Empty_Device {
public:
     virtual void read_reg(uint8_t &data, [[maybe_unused]]uint16_t addr) {
          return 0xff;
     }

     virtual void write_reg([[maybe_unused]]uint8_t data,
                            [[maybe_unused]]uint16_t addr) {
     }

     virtual void cycle() {
     }

     virtual uint8_t reg_base() {
         return 0;
     }

     virtual int reg_size() {
         return 128;
     } 
};
