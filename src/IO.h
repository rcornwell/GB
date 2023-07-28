/*
 * GB - IO space access.
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

#include "Memory.h"
#include "Device.h"

/**
 * @brief I/O Space memory object.
 *
 * Address space map:
 *
 *
 *   Address | Name   | Description           |
 *  -------- | -----  | ----------------------|
 *     0x00  | P1     | Buttons               |
 *     0x01  | SB     | Serial                |
 *     0x02  | SC     | ^                     |
 *     0x04  | DIV    | Timer                 |
 *     0x05  | TIMA   | ^                     |
 *     0x06  | TMA    | ^                     |
 *     0x07  | TAC    | ^                     |
 *     0x0F  | IF     | Interrupt register    |
 *     0x10  | NR 10  | Sound data            |
 *     0x11  | NR 11  | ^                     |
 *     0x12  | NR 12  | ^                     |
 *     0x13  | NR 13  | ^                     |
 *     0x14  | NR 14  | ^                     |
 *     0x16  | NR 21  | ^                     |
 *     0x17  | NR 22  | ^                     |
 *     0x18  | NR 23  | ^                     |
 *     0x19  | NR 24  | ^                     |
 *     0x1A  | NR 30  | ^                     |
 *     0x1B  | NR 31  | ^                     |
 *     0x1C  | NR 32  | ^                     |
 *     0x1D  | NR 33  | ^                     |
 *     0x1E  | NR 34  | ^                     |
 *     0x20  | NR 41  | ^                     |
 *     0x21  | NR 42  | ^                     |
 *     0x22  | NR 43  | ^                     |
 *     0x23  | NR 44  | ^                     |
 *     0x24  | NR 50  | ^                     |
 *     0x25  | NR 51  | ^                     |
 *     0x26  | NR 52  | ^                     |
 *     0x30-0x3F|     | Wave table.           |
 *     0x40  | LCDC   | Display controller    |
 *     0x41  | STAT   | ^                     |
 *     0x42  | SCY    | ^                     |
 *     0x43  | SCX    | ^                     |
 *     0x44  | LY (R) | ^                     |
 *     0x45  | LYC    | ^                     |
 *     0x46  | DMA    | ^                     |
 *     0x47  | BGP    | ^                     |
 *     0x48  | OBP0   | ^                     |
 *     0x49  | OBP1   | ^                     |
 *     0x4A  | WX     | ^                     |
 *     0x4B  | WY     | ^                     |
 *     0x50  | (W)    |Disable ROM            |
 *     0x7F-0xFE|     | Internal RAM.         |
 *     0xFF  | IE     | Interrupt enable      |
 *
 *
 * Access to I/O devices and internal RAM.
 */
class IO_Space : public Slice {
     uint8_t       _ram[128];         /**< Internal RAM */
     Device       *_devs[128];        /**< Pointer to devices */
     Empty_Device  _empty;            /**< Empty device to handle access to unused space */
     Device       *_irq_flg_dev;      /**< IRQ flags device */

     uint8_t      *_irq_en;           /**< Pointer to interrupt enable registers */
     uint8_t      *_irq_flg;          /**< Pointer to interrupt flags register */

public:

     IO_Space(uint8_t *irq_en, uint8_t *irq_flg) :
             _irq_en(irq_en), _irq_flg(irq_flg) {
         for(int i = 0; i < 128; i++) {
             _ram[i] = 0xff;
             _devs[i] = &_empty;
         }
         _irq_flg_dev = new Byte_Device(0xf, _irq_flg, 0xe0);
         _devs[_irq_flg_dev->reg_base()] = _irq_flg_dev;
     }

     ~IO_Space() {
         delete _irq_flg_dev;
     }

     /* We don't want to be able to copy this object */
     IO_Space(IO_Space &) = delete;

     IO_Space& operator=(IO_Space &) = delete;

     /**
      * @brief Size of I/O space in 256 bytes.
      *
      * @return 1
      */
     virtual size_t size() const override  { return 1; }

     /**
      * @brief Return bus number of slice.
      *
      * Used to manage DMA transfers the bus number needs to
      * be compared with the transfer page to determine how
      * access will occur.
      *
      * Bus number 0 has ROM, and external memory.
      * Bus number 1 has video ram.
      * Bus number 2 has OAM memory.
      * Bus number 3 is internal to the CPU.
      *
      * @return Bus number.
      */
     virtual int bus() const override { return 3; }

     /**
      * @brief Add a device to IO space.
      *
      * Add a device to I/O Space. Ask device for first address and number
      * of locatons. Also pass interrupt flag location to device to handle
      * interrupts.
      *
      * @param dev Device to add.
      */
     void add_device(Device *dev) {
         uint8_t   base = dev->reg_base();
         int       sz = dev->reg_size();

         dev->set_irq(_irq_flg);
         for (int i = 0; i < sz; i++) {
             _devs[base+i] = dev;
         }
      }

     /**
      * @brief Read from devices.
      *
      * Read from devices using their read routine.
      * @param[out] data Value read from the device.
      * @param[in] addr Address of I/O register or internal memory.
      */
     virtual void read(uint8_t& data, uint16_t addr) const override {
         data = 0xff;

         /* Access RAM or I/O device */
         if ((addr & 0x80)) {
              if ((addr & 0xff) == 0xff) {
                  data = *_irq_en;
              } else {
                  data = _ram[addr & 0x7f];
              }
         } else {
              _devs[(addr & 0x7f)]->read_reg(data, addr);
         }
     }

     /**
      * @brief Write to devices.
      *
      * Write to devices using their write routine.
      * @param[in] data Value write to the device.
      * @param[in] addr Address of I/O register or internal memory.
      */
     virtual void write(uint8_t data, uint16_t addr) override {
         /* Access RAM or I/O device */
         if ((addr & 0x80)) {
              if ((addr & 0xff) == 0xff) {
                  *_irq_en = data;
              } else {
                  _ram[addr & 0x7f] = data;
              }
         } else {
              _devs[(addr & 0x7f)]->write_reg(data, addr);
         }
    }
};
