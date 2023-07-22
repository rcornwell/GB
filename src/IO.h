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
#include "Joypad.h"
#include "Timer.h"
#include "Cartridge.h"
#include "Apu.h"
#include "Ppu.h"
#include "Serial.h"

/**
 * @brief I/O Space memory object.
 *
 * Access to I/O devices and internal RAM.
 */
class IO_Space : public Slice {
     uint8_t     ram[128];          /**< Internal RAM */

     uint8_t     *irq_en;           /**< Pointer to interrupt enable registers */
     uint8_t     *irq_flg;          /**< Pointer to interrupt flags register */

     Timer       *timer;            /**< Timer object */
     Cartridge   *cart;             /**< Cartridge object */
     Ppu         *ppu;              /**< PPU object */
     Joypad      *joy;              /**< Joypad object */
     Apu         *apu;              /**< Audio processing object */
     Serial      *serial;           /**< Game link */
public:

     IO_Space() : irq_en(NULL), irq_flg(NULL), timer(NULL), cart(NULL), ppu(NULL), 
              joy(NULL), apu(NULL), serial(NULL) {
         for(int i = 0; i < 128; i++)
             ram[i] = 0xff;
     }

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
      * @brief Hook up the interrupt register.
      *
      * Location to modify/read for interrupt handling.
      * These pointers are passed off to the various
      * devices that need them.
      *
      * @param irq_enb Pointer to interrupt enable bits.
      * @param irq_flag Pointer to interrupt flag bits.
      */
     void set_irq(uint8_t *irq_enb, uint8_t *irq_flag) {
          irq_en = irq_enb;
          irq_flg = irq_flag;
          if (timer) {
              timer->set_irq(irq_flag);
          }
          if (ppu) {
              ppu->set_irq(irq_flag);
          }
          if (joy) {
              joy->set_irq(irq_flag);
          }
          if (apu) {
              apu->set_irq(irq_flag);
          }
          if (serial) {
              serial->set_irq(irq_flag);
          }
     }

     /**
      * @brief Set pointer to Cartridge.
      *
      * Used to disable boot rom.
      * @param _cart Pointer to Cartridge Object.
      */
     void set_cart(Cartridge *_cart) {
          cart = _cart;
     }

     /**
      * @brief Set pointer to PPU.
      *
      * Used to access PPU registers.
      * @param _ppu Pointer to PPU Object.
      */
     void set_ppu(Ppu *_ppu) {
          ppu = _ppu;
     }

     /**
      * @brief Set pointer to Timer Object.
      *
      * Used to access timer registers.
      * @param _timer Pointer to Timer Object.
      */
     void set_timer(Timer *_timer) {
          timer = _timer;
     }

     /**
      * @brief Set pointer to Joypad.
      *
      * Used to access the Joypad register.
      * @param _joy Pointer to Joypad Object.
      */
     void set_joypad(Joypad *_joy) {
          joy = _joy;
     }

     /**
      * @brief Set pointer to APU.
      *
      * Used to access the Audio processor.
      * @param _apu Pointer to APU Object.
      */
     void set_apu(Apu *_apu) {
          apu = _apu;
     }

     /**
      * @brief Set pointer to Serial game link.
      *
      * Used to access the Serial game link.
      * @param _serial Pointer to Serial Object.
      */
     void set_serial(Serial *_serial) {
          serial = _serial;
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

         switch(addr & 0xff) {
         case 0x00:    /* P1 */   /* Buttons */
                       joy->read(data, addr);
                       break;
         case 0x01:    /* SB */   /* Serial */
         case 0x02:    /* SC */
                       serial->read(data, addr);
                       break;
         case 0x04:    /* DIV */  /* Timer */
         case 0x05:    /* TIMA */
         case 0x06:    /* TMA */
         case 0x07:    /* TAC */
                       timer->read(data, addr);
                       break;
         case 0x0F:    /* IF */   /* Interrupt register */
                       data = *irq_flg | 0xe0;
                       break;
         case 0xFF:    /* IE */   /* Interrupt enable */
                       data = *irq_en;
                       break;
         case 0x10:    /* NR 10 */  /* Sound data */
         case 0x11:    /* NR 11 */
         case 0x12:    /* NR 12 */
         case 0x13:    /* NR 13 */
         case 0x14:    /* NR 14 */
         case 0x16:    /* NR 21 */
         case 0x17:    /* NR 22 */
         case 0x18:    /* NR 23 */
         case 0x19:    /* NR 24 */
         case 0x1A:    /* NR 30 */
         case 0x1B:    /* NR 31 */
         case 0x1C:    /* NR 32 */
         case 0x1D:    /* NR 33 */
         case 0x1E:    /* NR 34 */
         case 0x20:    /* NR 41 */
         case 0x21:    /* NR 42 */
         case 0x22:    /* NR 43 */
         case 0x23:    /* NR 44 */
         case 0x24:    /* NR 50 */
         case 0x25:    /* NR 51 */
         case 0x26:    /* NR 52 */
         case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
         case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3E: case 0x3F:
                      apu->read(data, addr);
                      break;
                       
         case 0x40:   /* LCDC */  /* Display controller */
         case 0x41:   /* STAT */
         case 0x42:   /* SCY */
         case 0x43:   /* SCX */
         case 0x44:   /* LY (R) */
         case 0x45:   /* LYC */
         case 0x46:   /* DMA (W) */
         case 0x47:   /* BGP */
         case 0x48:   /* OBP0 */
         case 0x49:   /* OBP1 */
         case 0x4A:   /* WX */
         case 0x4B:   /* WY */
                      if (ppu) {
                         ppu->read(data, addr);
                      }
                      break;
         case 0x50:   /* Disable ROM */
                      break;
         case 0x03:    /* Empty */
         case 0x08:    /* Empty */
         case 0x09:    /* Empty */
         case 0x0A:    /* Empty */
         case 0x0B:    /* Empty */
         case 0x0C:    /* Empty */
         case 0x0D:    /* Empty */
         case 0x0E:    /* Empty */
         case 0x15:    /* Empty */
         case 0x1F:    /* Empty */
         case 0x27:    /* Empty */
         case 0x28:    /* Empty */
         case 0x29:    /* Empty */
         case 0x2A:    /* Empty */
         case 0x2B:    /* Empty */
         case 0x2C:    /* Empty */
         case 0x2D:    /* Empty */
         case 0x2E:    /* Empty */
         case 0x2F:    /* Empty */
         case 0x4C: case 0x4D: case 0x4E: case 0x4F:
         case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
         case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F:
         case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
         case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6E: case 0x6F:
         case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
         case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F:
                      data = 0xff;
                      break;
         default:
                      data = ram[addr & 0x7f];
                      break;
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
         switch(addr & 0xff) {
         case 0x00:    /* P1 */   /* Buttons */
                       joy->write(data, addr);
                       break;
         case 0x01:    /* SB */   /* Serial */
         case 0x02:    /* SC */
                       serial->write(data, addr);
                       break;
         case 0x04:    /* DIV */  /* Timer */
         case 0x05:    /* TIMA */
         case 0x06:    /* TMA */
         case 0x07:    /* TAC */
                       timer->write(data, addr);
                       break;
         case 0x0F:    /* IF */   /* Interrupt register */
                       *irq_flg = data;
                       break;
         case 0xFF:    /* IE */   /* Interrupt enable */
                       *irq_en = data;
                       break;
         case 0x10:    /* NR 10 */  /* Sound data */
         case 0x11:    /* NR 11 */
         case 0x12:    /* NR 12 */
         case 0x13:    /* NR 13 */
         case 0x14:    /* NR 14 */
         case 0x16:    /* NR 21 */
         case 0x17:    /* NR 22 */
         case 0x18:    /* NR 23 */
         case 0x19:    /* NR 24 */
         case 0x1A:    /* NR 30 */
         case 0x1B:    /* NR 31 */
         case 0x1C:    /* NR 32 */
         case 0x1D:    /* NR 33 */
         case 0x1E:    /* NR 34 */
         case 0x20:    /* NR 41 */
         case 0x21:    /* NR 42 */
         case 0x22:    /* NR 43 */
         case 0x23:    /* NR 44 */
         case 0x24:    /* NR 50 */
         case 0x25:    /* NR 51 */
         case 0x26:    /* NR 52 */
         case 0x30: case 0x31: case 0x32: case 0x33:
         case 0x34: case 0x35: case 0x36: case 0x37:
         case 0x38: case 0x39: case 0x3A: case 0x3B:
         case 0x3C: case 0x3D: case 0x3E: case 0x3F:
                      apu->write(data, addr);
                      break;
                       
         case 0x40:   /* LCDC */  /* Display controller */
         case 0x41:   /* STAT */
         case 0x42:   /* SCY */
         case 0x43:   /* SCX */
         case 0x44:   /* LY (R) */
         case 0x45:   /* LYC */
         case 0x46:   /* DMA (W) */
         case 0x47:   /* BGP */
         case 0x48:   /* OBP0 */
         case 0x49:   /* OBP1 */
         case 0x4A:   /* WX */
         case 0x4B:   /* WY */
                      if (ppu) {
                         ppu->write(data, addr);
                      }
                      break;
         case 0x50:   /* Disable ROM */
                      if (cart) {
                         cart->disable_rom(data);
                      }
                      break;
         case 0x03:    /* Empty */
         case 0x08:    /* Empty */
         case 0x09:    /* Empty */
         case 0x0A:    /* Empty */
         case 0x0B:    /* Empty */
         case 0x0C:    /* Empty */
         case 0x0D:    /* Empty */
         case 0x0E:    /* Empty */
         case 0x15:    /* Empty */
         case 0x1F:    /* Empty */
         case 0x27:    /* Empty */
         case 0x28:    /* Empty */
         case 0x29:    /* Empty */
         case 0x2A:    /* Empty */
         case 0x2B:    /* Empty */
         case 0x2C:    /* Empty */
         case 0x2D:    /* Empty */
         case 0x2E:    /* Empty */
         case 0x2F:    /* Empty */
         case 0x4C: case 0x4D: case 0x4E: case 0x4F:
         case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
         case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F:
         case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
         case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6E: case 0x6F:
         case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
         case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F:
                      break;
         default:
                      ram[addr & 0x7f] = data;
                      break;
        }
    }
};
