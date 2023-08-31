/*
 * GB - MBC3 Cartridge Mapper logic.
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
#include <ctime>
#include <cmath>
#include <functional>
#include "signal.h"

#include "Cartridge.h"
#include "MBC3.h"

#define RTC_S      0
#define RTC_M      4
#define RTC_H      8
#define RTC_DL     12
#define RTC_DH     16
#define RTC_LATCH  20
#define RTC_TIME   40

/**
 * @brief Load a 64 bit little endian number from data.
 *
 * @param p[in] Pointer to little endian data in memory.
 * @param x[out] 64 bit integer.
 */

static inline void load64_int(const uint8_t* p, uint64_t &x) {
    x = (uint64_t(p[7]) << 8*0) |
        (uint64_t(p[6]) << 8*1) |
        (uint64_t(p[5]) << 8*2) |
        (uint64_t(p[4]) << 8*3) |
        (uint64_t(p[3]) << 8*4) |
        (uint64_t(p[2]) << 8*5) |
        (uint64_t(p[1]) << 8*6) |
        (uint64_t(p[0]) << 8*7);
}

/**
 * @brief Store a 64 bit number as a little endian byte array.
 *
 * @param p[out] Pointer to were to store number.
 * @param x[in] 64 bit integer.
 */
static inline void store64_int(uint8_t *p, const uint64_t &x) {
    p[0] = (uint8_t)((x >> 8*0) & 0xff);
    p[1] = (uint8_t)((x >> 8*1) & 0xff);
    p[2] = (uint8_t)((x >> 8*2) & 0xff);
    p[3] = (uint8_t)((x >> 8*3) & 0xff);
    p[4] = (uint8_t)((x >> 8*4) & 0xff);
    p[5] = (uint8_t)((x >> 8*5) & 0xff);
    p[6] = (uint8_t)((x >> 8*6) & 0xff);
    p[7] = (uint8_t)((x >> 8*7) & 0xff);
}



/**
 * @brief Routine to read from memory.
 *
 * Return the value based on the mask to select range of access..
 * @param[out] data Data read from memory.
 * @param[in] addr Address of memory to read.
 */
void Cartridge_MBC3_RAM::read(uint8_t &data, uint16_t addr) const {
    switch(_bank) {
    case 0x0:   data = _data[(addr & _mask)]; break;
    case 0x1:   data = _data[(addr & _mask) | 0x2000]; break;
    case 0x2:   data = _data[(addr & _mask) | 0x4000]; break;
    case 0x3:   data = _data[(addr & _mask) | 0x6000]; break;
    default:    data = 0xff; break;
    case 0x8:
                if (_latched) {
                    data = _data[_rtc_base + RTC_S + RTC_LATCH];
                } else {
                    data = _data[_rtc_base + RTC_S];
                }
                data &= 0x3f;
                break;
    case 0x9:
                if (_latched) {
                    data = _data[_rtc_base + RTC_M + RTC_LATCH];
                } else {
                    data = _data[_rtc_base + RTC_M];
                }
                data &= 0x3f;
                break;
    case 0xa:
                if (_latched) {
                    data = _data[_rtc_base + RTC_H + RTC_LATCH];
                } else {
                    data = _data[_rtc_base + RTC_H];
                }
                data &= 0x1f;
                break;
    case 0xb:
                if (_latched) {
                    data = _data[_rtc_base + RTC_DL + RTC_LATCH];
                } else {
                    data = _data[_rtc_base + RTC_DL];
                }
                break;
    case 0xc:
                if (_latched) {
                    data = _data[_rtc_base + RTC_DH + RTC_LATCH];
                } else {
                    data = _data[_rtc_base + RTC_DH];
                }
                data &= 0xc1;
                break;
    }
}

/**
 * @brief Routine to write to memory.
 *
 * Set memory at address to data, based on mask.
 * @param[in] data Data to write to memory.
 * @param[in] addr Address of memory to write.
 */
void Cartridge_MBC3_RAM::write(uint8_t data, uint16_t addr) {
    switch(_bank) {
    case 0x0:   _data[(addr & _mask)] = data; break;
    case 0x1:   _data[(addr & _mask) | 0x2000] = data; break;
    case 0x2:   _data[(addr & _mask) | 0x4000] = data; break;
    case 0x3:   _data[(addr & _mask) | 0x6000] = data; break;
    default:    break;
    case 0x8:
                _data[_rtc_base + RTC_S] = data;
                break;
    case 0x9:
                _data[_rtc_base + RTC_M] = data;
                break;
    case 0xa:
                _data[_rtc_base + RTC_H] = data;
                break;
    case 0xb:
                _data[_rtc_base + RTC_DL] = data;
                break;
    case 0xc:
                _data[_rtc_base + RTC_DH] = data;
                break;
    }
}

uint8_t *Cartridge_MBC3_RAM::ram_data() {
    time_t       _time = time(NULL);

    store64_int(&_data[_rtc_base + RTC_TIME], (uint64_t)_time);
    return _data;
}

void Cartridge_MBC3_RAM::latch(uint8_t data) {
    /* Check if we should update the latched data. */
    if (_latched == 0 && (data & 1) != 0) {
        for (int i = 0; i < RTC_LATCH; i += 4) {
            _data[_rtc_base + i + RTC_LATCH] = _data[_rtc_base + i];
        }
    }
    _latched = (data & 1);
}

void Cartridge_MBC3_RAM::tick() {
    uint8_t      vs;
    uint8_t      vf;

    /* Check if counter is stoped */
    vf = _data[_rtc_base + RTC_DH];
    if ((vf & 0x40) != 0) {
        return;
    }

    vs = (_data[_rtc_base + RTC_S] + 1) & 0x3f;
    if (vs == 60) {
         vs = 0;
         uint8_t vm = (_data[_rtc_base + RTC_M] + 1) & 0x3f;
         if (vm == 60) {
             vm = 0;
             uint8_t vh = (_data[_rtc_base + RTC_H] + 1) & 0x1f;
             if (vh == 24) {
                vh = 0;
                advance_day();
             }
             _data[_rtc_base + RTC_H] = vh;
         }
         _data[_rtc_base + RTC_M] = vm;
     }
     _data[_rtc_base + RTC_S] = vs;
}

void Cartridge_MBC3_RAM::update_time() {
    time_t       _time = time(NULL);
    uint64_t     _ot;
    time_t       _oldtime;
    uint8_t      vf;
    uint8_t      v;

    /* Check if counter is stoped */
    vf = _data[_rtc_base + RTC_DH];
    if ((vf & 0x40) != 0) {
        return;
    }

    load64_int(&_data[_rtc_base + RTC_TIME], _ot);
    _oldtime = (time_t)_ot;
    double seconds = difftime(_time, _oldtime);
    /* If over 512 days, force overflow */
    if (seconds > (512 * 86400.0f)) {
        uint8_t v2 = _data[_rtc_base + RTC_DH];
        v2 |= 0x80;
        _data[_rtc_base + RTC_DH] = v2;
        /* Take remainder */
        seconds = std::remainder(seconds, (double)(512 * 86400.0f));
    }
    /* Advance by days until less than day remaining */
    while(seconds > 86400.0f) {
         advance_day();
         seconds -= 86400.0f;
    }

    /* Now advance by hours. */
    while(seconds > 3600.0f) {
         v = (_data[_rtc_base + RTC_H] + 1) & 0x1f;
         if (v == 24) {
             v = 0;
             advance_day();
         }
         _data[_rtc_base + RTC_H] = v;
         seconds -= 3600.0f;
    }

    /* Now advance by Minutes. */
    while(seconds > 60.0f) {
         v = (_data[_rtc_base + RTC_M] + 1) & 0x3f;
         if (v == 60) {
             v = 0;
             uint8_t vh = (_data[_rtc_base + RTC_H] + 1) & 0x1f;
             if (vh == 24) {
                vh = 0;
                advance_day();
             }
             _data[_rtc_base + RTC_H] = vh;
         }
         _data[_rtc_base + RTC_M] = v;
         seconds -= 60.0f;
    }

    /* Finally advance by seconds */
    while(seconds > 0.0f) {
         tick();
         seconds -= 1.0f;
    }
}

void Cartridge_MBC3_RAM::advance_day() {
    uint8_t      v;

    v = (_data[_rtc_base + RTC_DL] + 1) & 0xff;
    if (v == 0) {
        uint8_t v2 = _data[_rtc_base + RTC_DH] + 1;
        if ((v2 & 2) != 0) {
            v2 &= 0xfd;
            v2 |= 0x80;
        }
        _data[_rtc_base + RTC_DH] = v2;
    }
    _data[_rtc_base + RTC_DL] = v;
}

void Cartridge_MBC3_dis::write(uint8_t data, uint16_t addr) {
    if ((addr & 0x100) == 0) {
        _reg = data & 3;
    } else {
        if (_latched) {
            return;
        }
        switch(_reg) {
        case 0:     if (data == 0xc0) {
                        _latched = true;
                    };
                    break;
        /* It is unclear what this does, it appears to be mask */
        case 1:     _mask = data; break;
        /* It is unclear what is being set here */
        case 2:     _two = data; break;
        /* This appears to set the base address of bank 0 */
        case 3:     top->bank_zero = ((uint32_t)data) << 15;
                    break;
        }
    }
}

void Cartridge_MBC3_bank::write(uint8_t data, uint16_t addr) {
    /* Preform bank switching */
    switch (addr >> 13) {
    case 2:          /* 0x4000 - 0x5fff */
            /* MBC3   select ram bank */
            _ram_bank = ((uint32_t)(data & 0xf));
            if (_ram) {
                _ram->set_bank(_ram_bank);
            }
            break;
    case 3:          /* 0x6000 - 0x7fff */
            /* MBC3   Latch RTC */
            if (rtc_flag) {
                Cartridge_MBC3_RAM* ram =
                                  dynamic_cast<Cartridge_MBC3_RAM*>(_ram);
                ram->latch(data);
            }
            break;
    }
}

/**
 * @brief Setup RAM for Cartridge.
 *
 * Use values in ROM to allocate space for on Cartridge RAM.
 * If there is already RAM make sure it is same size as
 * cartridge defined RAM.
 */
Cartridge_RAM *Cartridge_MBC3::set_ram(int type, uint8_t *ram_data,
                 size_t ram_size) {
#define K  1024
     Cartridge_MBC3_RAM  *mbc_ram = NULL;
     size_t     size;

     /* Compute size of RAM based on ROM data */
     switch (_data[0x149]) {
     default:
     case 0:    size = 0; break;
     case 1:    size = 2*K; break;
     case 2:    size = 8*K; break;
     case 3:    size = 32*K; break;
     case 4:    size = 128*K; break;
     case 5:    size = 64*K; break;
     }

     if ((type & TIM) != 0) {
         _rom_bank->rtc_flag = true;
         if (ram_data != NULL) {
             uint32_t temp = (uint32_t)ram_size;
             if ((temp & (temp - 1)) != 0) {
                uint32_t extra = 48;
                temp |= temp >> 1;
                temp |= temp >> 2;
                temp |= temp >> 4;
                temp |= temp >> 8;
                temp |= temp >> 16;
                temp = temp ^ (temp >> 1);
                if (temp < 2*K) {
                    temp = 0;
                }
                extra = (uint32_t)ram_size - temp;
                if (extra != 44 && extra != 48) {
                    std::cerr << "Invalid RAM size " << (int)temp <<
                            " " << (int)extra << std::endl;
                }
                ram_size -= extra;
             }
         }
     }

     /* If we already have RAM data, try and use it */
     if (ram_data != NULL) {
         if (size != ram_size) {
             std::cerr << "Invalid Save file size: " << ram_size <<
               " Cartridge says: " << size << std::endl;
             exit(1);
         }
         if ((type & TIM) != 0) {
             mbc_ram = new Cartridge_MBC3_RAM(ram_data, ram_size);
             _ram = mbc_ram;
         } else {
             _ram = new Cartridge_RAM(ram_data, ram_size);
         }
     } else {
         if ((type & TIM) != 0) {
             mbc_ram = new Cartridge_MBC3_RAM(size);
             _ram = mbc_ram;
         } else {
             _ram = new Cartridge_RAM(size);
         }
     }

     if ((type & TIM) != 0) {
         if (mbc_ram != NULL) {
             mbc_ram->update_time();
         }
     }
     return _ram;
}

void Cartridge_MBC3::map_cart() {
    _rom_bank->set_ram(_ram);
    /* Map ourselves in place */
    _mem->add_slice(this, 0);
    _mem->add_slice(_rom_bank, 0x4000);
    _mem->add_slice_sz(&_empty, 0xa000, 32);
    disable_rom(0);
}

void Cartridge_MBC3::write(uint8_t data, uint16_t addr) {
    uint32_t    new_bank;
    /* Preform bank switching */
    switch (addr >> 13) {
    case 0:          /* 0x0000 - 0x1fff */
            /* MBC3    enable ram. */
            /*       0xa    - enable. else disable */
            if ((data & 0xc0) != 0) {
                _extra_banks = true;
                _mem->add_slice(&_dis_ram, 0xa000);
                return;
            }
            if (_ram == NULL)
                return;
            if ((data & 0xf) == 0xa) {
               _mem->add_slice(_ram, 0xa000);
            } else {
               _mem->add_slice_sz(&_empty, 0xa000, 32);
            }
            break;
    case 1:          /* 0x2000 - 0x3fff */
            /* MBC3   select memory bank */
            /*       0 - 7F   bank for 0x4000-0x7fff */
            new_bank = ((uint32_t)(data & 0xff)) << 14;
            if (new_bank == 0) {
                new_bank = 0x4000;
            }
            new_bank += bank_zero;
            new_bank &= _size - 1;
            _rom_bank->set_bank(new_bank);
            break;
     }
}

