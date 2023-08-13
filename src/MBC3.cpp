/*
 * GB - MBC3 Cartidge Mapper logic.
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
#include <functional>
#include <endian.h>
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
    uint64_t     *t = (uint64_t *)&_data[_rtc_base + RTC_TIME];
    time_t       _time = time(NULL);

    *t = htole64((uint64_t)_time);
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
    uint64_t     *t = (uint64_t *)&_data[_rtc_base + RTC_TIME];
    time_t       _time = time(NULL);
    time_t       _oldtime;
    uint8_t      vf;
    uint8_t      v;

    /* Check if counter is stoped */
    vf = _data[_rtc_base + RTC_DH];
    if ((vf & 0x40) != 0) {
        return;
    }

    _oldtime = le64toh(*t);
    double seconds = difftime(_time, _oldtime);
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

    /* Finaly advance by seconds */
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

/**
 * @brief Setup RAM for Cartridge.
 *
 * Use values in ROM to allocate space for on Cartridge RAM.
 * If there is already RAM make sure it is same size as
 * cartridge defined RAM.
 */
Cartridge_RAM *Cartridge_MBC3::set_ram(int type, uint8_t *ram_data, size_t ram_size) {
#define K  1024
     Cartridge_MBC3_RAM  *mbc_ram;
     size_t     size;

     /* Compute size of RAM based on ROM data */
     switch (_data[0x149]) {
     default:
     case 0:    size = 0; break;
     case 1:    size = 2*K; break;
     case 2:    size = 8*K; break;
     case 3:    size = 32*K; break;
     }

     if ((type & TIM) != 0) {
         _rom_bank->rtc_flag = true;
         if (ram_data != NULL) {
             uint16_t temp = ram_size;
             if ((temp & (temp - 1)) != 0) {
                uint16_t extra = 48;
                temp |= temp >> 1;
                temp |= temp >> 2;
                temp |= temp >> 4;
                temp |= temp >> 8;
                temp |= temp >> 16;
                temp = temp ^ (temp >> 1);
                if (temp < 2*K) {
                    temp = 0;
                }
                extra = ram_size - temp;
                if (extra != 44 && extra != 48) {
                    printf("Invalid RAM size %d %d\n", temp, extra);
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
         if (ram_data != NULL) {
             mbc_ram->update_time();
         }
     }
     return _ram;
}

