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

void Memory::cycle() {
     _cycles++;
     _timer->cycle();
     _serial->cycle();
     _ppu->cycle();
     _apu->cycle();
}

void Memory::read(uint8_t &data, uint16_t addr) {
     _apu->cycle_early();
     if (_dma_flag) {
        /* If DMA in progress, wait 2 cycles before transfering */
        if (++_dma_count >= 0) {
            /* Transfer the data */
            _mem[(_dma_addr >> 8) & 0xff]->read(_dma_data, _dma_addr | _dma_count);
            _mem[0xfe]->write(_dma_data, _dma_count);
            /* Stop at 160 bytes transfered */
            if (_dma_count == 0x9f) {
                _dma_flag = 0;
            }
//printf("DMA Read %04x %04x %02x %d\n", addr, (_dma_addr & 0xff00) | _dma_count, _dma_data, _dma_count);
            if ((addr & 0xff00) == 0xfe00) {
                data = 0xff;
                cycle();
                return;
            }
            /* If CPU attempts to access memory during DMA, 
             * it returns DMA data instead of date.
             */
            if (_mem[(addr >> 8) & 0xff]->bus() == _dma_bus) {
                data = _dma_data;
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
     _apu->cycle_early();
     if (_dma_flag) {
        /* If DMA in progress, wait 2 cycles before transfering */
        if (++_dma_count >= 0) {
            /* Transfer the data */
            _mem[(_dma_addr >> 8) & 0xff]->read(_dma_data, _dma_addr | _dma_count);
            _mem[0xfe]->write(_dma_data, _dma_count);
            /* Stop at 160 bytes transfered */
            if (_dma_count == 0x9f) {
                _dma_flag = 0;
            }
//printf("DMA Write %04x %04x %02x %d\n", addr, (_dma_addr & 0xff00) | _dma_count, _dma_data, _dma_count);
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
     _apu->cycle_early();
     if (_dma_flag) {
        /* If DMA in progress, wait 2 cycles before transfering */
        if (++_dma_count >= 0) {
            uint8_t   data;
            /* Transfer the data */
            _mem[(_dma_addr >> 8) & 0xff]->read(data, _dma_addr | _dma_count);
            _mem[0xfe]->write(data, _dma_count);
//printf("DMA Internal %04x %02x %d\n", (_dma_addr & 0xff00) | _dma_count, data, _dma_count);
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
    
    for (size_t i = 0; i < sz; i++) {
        _mem[sl+i] = slice;
    }
}

void Memory::add_slice(Slice *slice, uint16_t base) {
    int     sl = (base >> 8) & 0xff;
    size_t  sz = slice->size();
    
    for (size_t i = 0; i < sz; i++) {
        _mem[sl+i] = slice;
    }
}

void Memory::free_slice(uint16_t base) {
    int     sl = (base >> 8) & 0xff;
    size_t  sz = _mem[sl]->size();

    for (size_t i = 0; i < sz; i++) {
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
