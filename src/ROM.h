/*
 * GB - Boot ROM
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


#include <cstdint>
#include "Memory.h"

/**
 * @brief Boot ROM for Game Boy.
 *
 * This ROM holds the boot rom. The ROM is located at location 0 on
 * reset. Later the ROM can be disabled before transfering to the game
 * cartridge.
 */
class ROM : public Slice {
    const  static uint8_t rom_data[256];    /**< Boot rom contents */
public:

    /**
     * @brief Read from Boot ROM.
     *
     * Read from boot ROM, only look at 8 lower bits.
     *
     * @param[out] data Data read from ROM.
     * @param[in] addr  Location to read data from.
     */
    virtual void read(uint8_t &data, uint16_t addr) const override {
       data = rom_data[addr & 0xff];
    }

    /**
     * @brief Write a byte to Boot ROM.
     *
     * ROM's can't be written so default is to do nothing.
     *
     * @param[in] data Value to write to memory at address.
     * @param[in] addr Addess to access.
     */
    virtual void write([[maybe_unused]]uint8_t data,
                       [[maybe_unused]]uint16_t addr) override {
    }

    /**
     * @brief Return size of Boot ROM.
     *
     * Boot ROM is fixed at one page.
     * @return size of Boot ROM in pages.
     */
    virtual size_t size() const override {
       return 1;
    }

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

};
