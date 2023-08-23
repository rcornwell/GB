/*
 * GB - Main Memory objects.
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


#include <cstddef>
#include <cstdio>

#include "Memory.h"
#include "Timer.h"
#include "Ppu.h"
#include "Apu.h"
#include "Serial.h"

void Memory::switch_speed() {
     _speed = !_speed;
     _timer->set_speed(_speed);
}

void Memory::idle() {
     _cycles++;
}

void Memory::hdma_cycle() {
     if (hdma_en) {
         bool sav_speed = _speed;
         _speed = true;
//  printf("HDMA transfer from %02x %04x -> %04x\n", hdma_cnt, hdma_src, hdma_dst | 0x8000);
         for (int i = 0; i < 16; i++) {
             uint8_t dma_data;
             cycle();
             _mem[(hdma_src >> 8) & 0xff]->read(dma_data, hdma_src);
             _mem[((hdma_dst >> 8) & 0x1f) | 0x80]->write(dma_data, 0x8000 | hdma_dst);
//printf("HDMA %04x -> %04x %02x\n", hdma_src, 0x8000 | hdma_dst, dma_data);
             cycle();
             hdma_src++;
             hdma_dst = 0x1fff & (hdma_dst + 1);
         }
         hdma_cnt--;
         hdma_cnt &= 0x7f;
         if (hdma_cnt == 0x7f) {
             hdma_en = false;
         }
         _speed = sav_speed;
      }
}

void Memory::cycle() {
     switch(_step) {
     case 0:
             _ppu->dot_cycle();
             if (_speed) {
                 _step = 1;
                 return;
             }
             /* Fall through */
             [[fallthrough]];
     case 1:
             _apu->cycle_early();
             _ppu->dot_cycle();
             if (_speed) {
                 _timer->cycle();
             }
             _step = 2;
             break;
     case 2:
             _ppu->dot_cycle();
             if (_speed) {
                 _step = 3;
                 return;
             }
             /* Fall through */
             [[fallthrough]];
     case 3:
             _cycles++;
             _timer->cycle();
             _serial->cycle();
             _ppu->dot_cycle();
             _apu->cycle();
             _step = 0;
             break;
     }
}

void Memory::read(uint8_t &data, uint16_t addr) {
     cycle();
     if (_dma_flag) {
        /* If DMA in progress, wait 2 cycles before transfering */
        if (++_dma_count >= 0) {
            uint8_t   dma_data;
            /* Transfer the data */
            _mem[(_dma_addr >> 8) & 0xff]->read(dma_data, _dma_addr | _dma_count);
            _oam->write(dma_data, _dma_count);
            /* Stop at 160 bytes transfered */
            if (_dma_count == 0x9f) {
                _dma_flag = 0;
            }
//printf("DMA Read %04x %04x %02x %d\n", addr, (_dma_addr & 0xff00) | _dma_count, dma_data, _dma_count);
            if ((addr & 0xff00) == 0xfe00) {
                data = 0xff;
                cycle();
                return;
            }
            /* If CPU attempts to access memory during DMA,
             * it returns DMA data instead of date.
             */
            if (_mem[(addr >> 8) & 0xff]->bus() == _dma_bus) {
                data = dma_data;
                cycle();
                return;
            }
        }
     }
     /* Do actual read */
     _mem[(addr >> 8) & 0xff]->read(data, addr);
     cycle();
}

void Memory::write(uint8_t data, uint16_t addr) {
     cycle();
     if (_dma_flag) {
        /* If DMA in progress, wait 2 cycles before transfering */
        if (++_dma_count >= 0) {
            uint8_t   dma_data;
            /* Transfer the data */
            _mem[(_dma_addr >> 8) & 0xff]->read(dma_data, _dma_addr | _dma_count);
            _oam->write(dma_data, _dma_count);
            /* Stop at 160 bytes transfered */
            if (_dma_count == 0x9f) {
                _dma_flag = 0;
            }
//printf("DMA Write %04x %04x %02x %d\n", addr, (_dma_addr & 0xff00) | _dma_count, dma_data, _dma_count);
            if (_mem[(addr >> 8) & 0xff]->bus() == 2) {
                cycle();
                return;
            }
            if (_mem[(addr >> 8) & 0xff]->bus() == _dma_bus) {
                cycle();
                return;
            }
         }
     }
     /* Do actual write */
     _mem[(addr >> 8) & 0xff]->write(data, addr);
     cycle();
}

void Memory::internal() {
     cycle();
     if (_dma_flag) {
        /* If DMA in progress, wait 2 cycles before transfering */
        if (++_dma_count >= 0) {
            uint8_t   dma_data;
            /* Transfer the data */
            _mem[(_dma_addr >> 8) & 0xff]->read(dma_data, _dma_addr | _dma_count);
            _oam->write(dma_data, _dma_count);
//printf("DMA Internal %04x %02x %d\n", (_dma_addr & 0xff00) | _dma_count, dma_data, _dma_count);
            /* Stop at 160 bytes transfered */
            if (_dma_count == 0x9f) {
                _dma_flag = 0;
            }
         }
     }
     cycle();
}

void Memory::read_nocycle(uint8_t &data, uint16_t addr) {
     _mem[(addr >> 8) & 0xff]->read(data, addr);
}

void Memory::add_slice_sz(Slice *slice, uint16_t base, size_t sz) {
    int   sl = (base >> 8) & 0xff;

    for (size_t i = 0; i < sz && (i + sl) < 256; i++) {
        _mem[sl+i] = slice;
    }
}

void Memory::add_slice(Slice *slice, uint16_t base) {
    int     sl = (base >> 8) & 0xff;
    size_t  sz = slice->size();

    for (size_t i = 0; i < sz && (i + sl) < 256; i++) {
        _mem[sl+i] = slice;
    }
}

void Memory::free_slice(uint16_t base) {
    int     sl = (base >> 8) & 0xff;
    size_t  sz = _mem[sl]->size();

    for (size_t i = 0; i < sz && (i + sl) < 256; i++) {
        _mem[sl+i] = &_empty;
    }
}

void Memory::write_dma(uint8_t data) {
    /* If top memory, use work RAM */
    if ((data & 0xe0) == 0xe0) {
        data &= 0xdf;
    }
    _dma_addr = ((uint16_t)data) << 8;
    /* Record Bus */
    _dma_bus = _mem[data]->bus();
    /* Wait 2 cycles before starting. */
    if (!_dma_flag) {
        _dma_flag = 1;
    _dma_count = -2;
    } else {
         _dma_count = -1;
    }
}

void Memory::read_dma(uint8_t& data) {
     data = (uint8_t)((_dma_addr >> 8) & 0xff);
}
