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

#define VBLANK_IRQ 0x01
#define PPU_IRQ    0x02
#define TIMER_IRQ  0x04
#define SERIAL_IRQ 0x08
#define JOYPAD_IRQ 0x10

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

     virtual ~Device () {}

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
      * @brief Post device specific interrupt.
      *
      * Set pointer to where interrupts need to be updated.
      *
      * @param value bit to set.
      */
     virtual void post_irq(uint8_t value) {
         if (_irq_flg) {
            *_irq_flg |= value;
         }
     }

    /**
     * @brief Process a single cycle.
     *
     */
     virtual void cycle() { }

     /**
      * @brief Read device register contents.
      *
      * @param[out] data Data read from device.
      * @param[in] addr Address to read from device.
      */
     virtual void read_reg(uint8_t &data, uint16_t addr) const = 0;

     /**
      * @brief Write device register contents.
      *
      * @param[in] data Data to write to device register.
      * @param[in] addr Address to write data to.
      */
     virtual void write_reg(uint8_t data, uint16_t addr) = 0;

     /**
      * @brief Returns the base address of the device.
      *
      * @return base address of device.
      */
     virtual uint8_t reg_base() const = 0;

     /**
      * @brief Returns the number of registers device occupies.
      *
      * @return number of registers.
      */
     virtual int reg_size() const = 0;
};

/**
 * @brief Empty device object.
 *
 * This is a generic device that does nothing.
 */
class Empty_Device : public Device {
public:
     /**
      * @brief Read device register contents.
      *
      * Empty devices always return 0xff.
      *
      * @param[out] data Data read from device.
      * @param[in] addr Address to read from device.
      */
     virtual void read_reg(uint8_t &data, [[maybe_unused]]uint16_t addr) const override {
          data = 0xff;
     }

     /**
      * @brief Write device register contents.
      *
      * There is nothing to write into an empty device.
      *
      * @param[in] data Data to write to device register.
      * @param[in] addr Address to write data to.
      */
     virtual void write_reg([[maybe_unused]]uint8_t data,
                            [[maybe_unused]]uint16_t addr) override {
     }

     /**
      * @brief Empty device fills lower half of address space.
      *
      * @return base address of device.
      */
     virtual uint8_t reg_base() const override {
         return 0;
     }

     /**
      * @brief Empty device fills lower half of address space.
      *
      * @return number of registers.
      */
     virtual int reg_size() const override {
         return 128;
     }
};


/**
 * @brief Device which has just a generic byte pointer as value.
 *
 * This device can only be read or written to and has length of 1.
 */
class Byte_Device : public Device {
     uint16_t      _addr;
     uint8_t       *_loc;
     uint8_t       _mask;
public:
     Byte_Device (uint16_t addr, uint8_t *loc, uint8_t mask = 0x00) :
                   _addr(addr), _loc(loc), _mask(mask) { }

     /**
      * @brief Read device register contents.
      *
      * Return the contents of the location.
      *
      * @param[out] data Data read from device.
      * @param[in] addr Address to read from device.
      */
     virtual void read_reg(uint8_t &data, [[maybe_unused]]uint16_t addr) const override {
          data = *_loc | _mask;
     }

     /**
      * @brief Write device register contents.
      *
      * Update location contents.
      *
      * @param[in] data Data to write to device register.
      * @param[in] addr Address to write data to.
      */
     virtual void write_reg(uint8_t data, [[maybe_unused]]uint16_t addr) override {
          *_loc = data;
     }

     /**
      * @brief Device has fixed address.
      *
      * @return base address of device.
      */
     virtual uint8_t reg_base() const override {
         return _addr & 0x7f;
     }

     /**
      * @brief Device is one location long.
      *
      * @return number of registers.
      */
     virtual int reg_size() const override {
         return 1;
     }
};
