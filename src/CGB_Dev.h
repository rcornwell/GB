/*
 * GB - Color Game Boy specific devices.
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

#include <stdint.h>
#include "Device.h"
#include "Memory.h"
#include "Ppu.h"

/**
 * @brief Ram bank control device.
 *
 * Device handles WRAM bank switching on color Game Boy.
 */
class SVBK : public Device {
    RAM      *_ram;         /**< Pointer to Ram memory */
    uint8_t   _bank;

public:
    explicit SVBK(RAM *ram) : _ram(ram), _bank(1) {}

    /**
     * @brief RAM bank control read register.
     *
     * Returns last bank selected.
     *
     * @param[out] data Data read from register.
     * @param[in] addr Address of register to read.
     */
     virtual void read_reg(uint8_t &data, [[maybe_unused]]uint16_t addr) const override {
          data = _bank;
     }

    /**
     * @brief RAM bank select register.
     *
     * Pass data to RAM object to set bank.
     *
     * @param[in] data Data write to register.
     * @param[in] addr Address of register to write..
     */
     virtual void write_reg(uint8_t data,
                            [[maybe_unused]]uint16_t addr) override {
          _bank = data;
          _ram->set_bank(data);
     }

     /**
      * @brief RAM bank select is one location.
      *
      * @return base address of device.
      */
     virtual uint8_t reg_base() const override {
         return 0x70;
     }

     /**
      * @brief RAM bank select is one location.
      *
      * @return number of registers.
      */
     virtual int reg_size() const override {
         return 1;
     }
};

/**
 * @brief Key registers to control speed and PPU modes.
 *
 * Device handles WRAM bank switching on color Game Boy.
 */
class KEY : public Device {
    Ppu      *_ppu;         /**< Pointer to PPU device */
    Memory   *_mem;         /**< Pointer to Memory device */
    uint8_t   _ppu_mode;    /**< Mode for PPU device. */
    bool      _dis_speed;   /**< Disable speed switching */

public:
    bool      sw_speed;     /**< Switch Speed */

    explicit KEY(Ppu *ppu, Memory *mem) : _ppu(ppu), _mem(mem) {
        _ppu_mode = 0;
        sw_speed = false;
        _dis_speed = false;
    }

    /**
     * @brief RAM bank control read register.
     *
     * Returns last bank selected.
     *
     * @param[out] data Data read from register.
     * @param[in] addr Address of register to read.
     */
     virtual void read_reg(uint8_t &data, uint16_t addr) const override {
          data = 0xff;
          if (_dis_speed) {
              return;
          }
          if ((addr & 1)) {
              data = (uint8_t)sw_speed;
              if (_mem->get_speed()) {
                  data |= 0x80;
              }
          } else {
              if (!_mem->get_disable()) {
                 data = _ppu_mode;
              }
          }
     }

    /**
     * @brief RAM bank select register.
     *
     * Pass data to RAM object to set bank.
     *
     * @param[in] data Data write to register.
     * @param[in] addr Address of register to write..
     */
     virtual void write_reg(uint8_t data, uint16_t addr) override {
          if (_dis_speed) {
              return;
          }
          if ((addr & 1)) {
             sw_speed = (data & 1);
          } else {
              if (!_mem->get_disable()) {
                 _ppu_mode = data;
                 _ppu->set_ppu_mode(data);
                 if ((data & 0x8) != 0) {
                     _dis_speed = true;
                     _mem->dis_hdma = true;
                 }
              }
          }
     }

     /**
      * @brief Base address for Key registers
      *
      * @return base address of device.
      */
     virtual uint8_t reg_base() const override {
         return 0x4c;
     }

     /**
      * @brief Size of Key registers
      *
      * @return number of registers.
      */
     virtual int reg_size() const override {
         return 2;
     }
};

/**
 * @brief Handle selection of video bank on Color Game Boy.
 *
 * Device handles VRAM bank switching on color Game Boy.
 */
class VBK : public Device {
    Ppu      *_ppu;         /**< Pointer to PPU device */
    uint8_t   _bank;        /**< Video bank cache */

public:
    explicit VBK(Ppu *ppu) : _ppu(ppu), _bank(0) { }

    /**
     * @brief RAM bank control read register.
     *
     * Returns last bank selected.
     *
     * @param[out] data Data read from register.
     * @param[in] addr Address of register to read.
     */
     virtual void read_reg(uint8_t &data, [[maybe_unused]]uint16_t addr) const override {
          data = 0xfe | _bank;
     }

    /**
     * @brief RAM bank select register.
     *
     * Pass data to RAM object to set bank.
     *
     * @param[in] data Data write to register.
     * @param[in] addr Address of register to write..
     */
     virtual void write_reg(uint8_t data,
                            [[maybe_unused]]uint16_t addr) override {
          _bank = data;
          _ppu->set_vbank(data);
     }

     /**
      * @brief Base address for VBK register
      *
      * @return base address of device.
      */
     virtual uint8_t reg_base() const override {
         return 0x4f;
     }

     /**
      * @brief Size of VBK register
      *
      * @return number of registers.
      */
     virtual int reg_size() const override {
         return 1;
     }
};

/**
 * @brief Color Hblank DMA registers.
 *
 */
class HDMA : public Device {
     Memory     *_mem;        /**< Pointer to Memory object */

public:
     explicit HDMA(Memory *mem) : _mem(mem) {}

    /**
     * @brief Read HDMA registers.
     *
     * Returns HDMA values.
     *
     * @param[out] data Data read from register.
     * @param[in] addr Address of register to read.
     */
     virtual void read_reg(uint8_t &data, uint16_t addr) const override {
          if (!_mem->dis_hdma && (addr & 07) == 5) {
              data = _mem->hdma_cnt;
              data |= (_mem->hdma_en) ? 0 : 0x80;
          } else {
              data = 0xff;
          }
     }

    /**
     * @brief Write HDMA registers.
     *
     * Update data in Memory for HDMA transfer.
     *
     * @param[in] data Data write to register.
     * @param[in] addr Address of register to write..
     */
     virtual void write_reg(uint8_t data, uint16_t addr) override {
          if (_mem->dis_hdma) {
              return;
          }
          switch(addr & 07) {
          case 1:
                  _mem->hdma_src = (((uint16_t)data) << 8) | (_mem->hdma_src & 0xf0);
                  break;
          case 2:
                  _mem->hdma_src = (_mem->hdma_src & 0xff00) | ((uint8_t)(data & 0xf0));
                  break;
          case 3:
                  _mem->hdma_dst = (((uint16_t)data) << 8) | (_mem->hdma_dst & 0xf0);
                  break;
          case 4:
                  _mem->hdma_dst = (_mem->hdma_dst & 0x1f00) | ((uint8_t)(data & 0xf0));
                  break;
          case 5:
                  _mem->hdma_cnt = data & 0x7f;
                  if (data & 0x80) {
                      _mem->hdma_en = true;
                  } else {
                      if (_mem->hdma_en) {
                          _mem->hdma_en = false;
                      } else {
                          _mem->hdma_en = true;
                          while(_mem->hdma_en) {
                              _mem->hdma_cycle();
                          }
                      }
                  }
          }
     }

     /**
      * @brief Address of HDMA device.
      *
      * @return base address of device.
      */
     virtual uint8_t reg_base() const override {
         return 0x51;
     }

     /**
      * @brief Size of HDMA device.
      *
      * @return number of registers.
      */
     virtual int reg_size() const override {
         return 5;
     }
};

/**
 * @brief Game Boy Object Priority mode.
 *
 * Manage the Object Priority mode bit.
 */
class OPRI : public Device {
     Ppu          *_ppu;            /**< Pointer to PPU device */
     uint8_t      _mode;            /**< Priority mode */

public:

     explicit OPRI(Ppu *ppu) : _ppu(ppu), _mode(0) { }

     /**
      * @brief Address of Object priority mode.
      *
      * @return base address of device.
      */
     virtual uint8_t reg_base() const override {
         return 0x6c;
     }

     /**
      * @brief Number of registers Object priority mode.
      *
      * @return number of registers.
      */
     virtual int reg_size() const override {
         return 1;
     }

     virtual void read_reg(uint8_t &data, uint16_t addr) const override {
         data = _mode | 0xfe;
     }

     virtual void write_reg(uint8_t data, uint16_t addr) override {
         _mode = data;
         _ppu->set_obj_pri(data);
     }
};

