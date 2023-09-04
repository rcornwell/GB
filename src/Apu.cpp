/*
 * GB - Default audio waveforms.
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

#include "Apu.h"

const uint8_t Sound::sq_wave[32] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f,   /* 12.5 % */
            0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f,   /* 25.0 % */
            0x0f, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x0f,   /* 50.0 % */
            0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00    /* 75.0 % */
};

const uint8_t Sound::int_wave[32] = {
            0x0a, 0x0c, 0x0d, 0x0d, 0x0d, 0x0a, 0x04, 0x08,
            0x03, 0x06, 0x00, 0x02, 0x0c, 0x0f, 0x01, 0x06,
            0x02, 0x0c, 0x00, 0x04, 0x0e, 0x05, 0x02, 0x0c,
            0x0a, 0x0c, 0x0d, 0x0d, 0x0d, 0x0a, 0x04, 0x08,
};

void Sound::start(bool trigger, bool prev_use_len, bool next_frame_length) {
    /* enabling use length and we are currently in length update, there
     * is an extra clock pulse to length counter
     */
    if (!prev_use_len && _use_len && next_frame_length && _length != 0) {
        _length--;
        /* If this sets length counter to zero disable channel */
        if (!trigger && _length == 0) {
            active = false;
        }
    }

    /* On triggering */
    if (trigger) {
        /* If length is currently zero force to max length */
        if (_length == 0) {
            _length = _max_length;
            /* If length enabled and currently updating length, extra
             * clock cycle */
            if (_use_len && next_frame_length) {
                _length--;
            }
        }
        /* If triggered and enable, start channel */
        if (_dac_enable) {
           active = true;
        }

        /* Load initial and shadow frequency */
        _freq_cnt = _int_freq;

        /* Set initial frequency and length to play */
        _volume = _int_vol;
        _pos = _wave_start;
        if (_int_vol_len != 0) {
            _vol_len = _int_vol_len;
            _vol_env = true;
        } else {
            _vol_len = 0;
            _vol_env = false;
        }
        _delay = true;
    }
}

/**
 * @brief reset channel.
 *
 * Stops any channel activity and set it to default state.
 */
void Sound::reset() {
    active = false;
    write_reg0(0);
    write_reg1(0);
    write_reg2(0);
    write_reg3(0);
    write_reg4(0,0);
}

/**
 * @brief Update channels length counter.
 *
 * Clock up length counter until it overflows, then stop channel.
 * If not enabled or not using length, do nothing.
 */
void Sound::update_length() {
    if (_use_len && _length != 0 && --_length <= 0) {
        if (active) {
            active = false;
            sample = 0;
            _vol_env = false;
        }
        _length = 0;
    }
}

/**
 * @brief Update channels volume sweep.
 */
void Sound::update_volume() {
    if (active && _vol_env) {
        if (_vol_len != 0) {
            _vol_len--;
        } else {
             int new_vol;
             if (_vol_dir) {
                 new_vol = _volume + 1;
             } else {
                 new_vol = _volume - 1;
             }
             if (new_vol >= 0 && new_vol <= 15) {
                 _volume = (uint8_t)new_vol;
                 _vol_len = _int_vol_len;
             } else {
                 _vol_env = false;
             }
        }
    }
}

/**
 * @brief Update channels frequency  sweep.
 *
 * Called to update the frequency at frequency sweep time. This is
 * only defined for channel 1.
 */
void Sound::update_sweep() {
}

/**
 * @brief Update frequency sequence counter for one clock pulse.
 *
 * We count the timer up until it overflows. At which time we select the
 * next wave sample and advance the sample, then we reset the timer.
 * Otherwise just wait until it overflows.
 */
void Sound::cycle() {
    if (_delay) {
        _delay = false;
        return;
    }
    if (active) {
        if (_freq_cnt & 0x800) {
            _freq_cnt = _int_freq;
            _pos++;
            if (_pos == _wave_end) {
                _pos = _wave_start;
            }
            if (_dac_enable) {
                sample = ((int8_t)_wave[_pos] - 8) * _volume;
            } else {
                sample = 0;
            }
            _delay = true;
        } else {
            _freq_cnt++;
        }
    } else {
        sample = 0;
    }
}

/**
 * @brief write register 0.
 *
 * Frequency sweep shift register.
 */
void Sound::write_reg0([[maybe_unused]]uint8_t data) {
}

/**
 * @brief write register 1.
 *
 * Sound length register.
 */
void Sound::write_reg1(uint8_t data) {
    if (!enabled) {
        return;
    }
    _length = _max_length - (data & 0x3f);
    _duty = (data >> 6) & 3;
    _wave_start = (_duty << 3);
    _wave_end = (_duty << 3) + 8;
}

/**
 * @brief write register 2.
 *
 * Volume envelop register.
 */
void Sound::write_reg2(uint8_t data) {
    if (!enabled) {
        return;
    }
    _int_vol = (data >> 4) & 0xf;
    _vol_dir = (data & 0x8) != 0;
    _int_vol_len = (data & 07);
    if (_int_vol == 0 && _vol_dir == 0) {
        active = false;
        _dac_enable = false;
    } else {
        _dac_enable = true;
    }
}

/**
 * @brief write register 3.
 *
 * Frequency register lower.
 */
void Sound::write_reg3(uint8_t data) {
    if (!enabled) {
        return;
    }
    _int_freq = (_int_freq & 0x700) | ((int)data & 0xff);
}

/**
 * @brief write register 4.
 *
 * Frequency register upper.
 */
void Sound::write_reg4(uint8_t data, int next_length) {
    if (!enabled) {
        return;
    }
    bool prev_use_len = _use_len;
    bool trigger = ((data & 0x80) != 0);
    bool next_frame_length = (next_length & 1) != 0;
    _int_freq = (((uint16_t)(data & 7) << 8) & 0x700) | (_int_freq & 0xff);
    _use_len = (data & 0x40) != 0;
    start(trigger, prev_use_len, next_frame_length);
    if (trigger) {
       sample = ((int8_t)_wave[_pos] - 8) * _volume;
    }
}



/**
 * @brief Calculate the next frequency with sweep.
 *
 * Depending on the value of _freq_dir either increment or
 * decrement the frequency.
 *
 * @return Then next frequency.
 */
uint16_t S1::next_freq() {
    uint16_t new_freq = _sweep_freq >> _sweep_shift;
    if (_sweep_dir) {
         new_freq = _sweep_freq - new_freq;
         _last_sub = true;
    } else {
         new_freq = _sweep_freq + new_freq;
    }
    return new_freq;
}

/**
 * @brief Update frequency.
 *
 * When update_sweep times out update frequency to new one.
 *
 * We do this by shifting the initial frequency right by
 * shift _freq_shift times, we then add or subtract this
 * value from initial frequency. If we overflow or under
 * flow set frequency to zero and stop the playback.
 */
void S1::update_step() {
    uint16_t new_freq = next_freq();
    if (new_freq >= 2048) {
        active = false;
        _shift_ena = false;
        sample = 0;
    } else {
        if (_sweep_shift) {
            _int_freq = _sweep_freq = new_freq;
        }
        if (next_freq() >= 2048) {
            active = false;
            _shift_ena = false;
            sample = 0;
        }
    }
}

/**
 * @brief Update channels frequency  sweep.
 */
void S1::update_sweep() {
    if (active && _shift_ena) {
         if (--_sweep_clk == 0) {
            if (_sweep_period != 0) {
                _sweep_clk = _sweep_period;
                update_step();
            } else {
                _sweep_clk = 8;
            }
        }
    }
}


/**
 * @brief write register 0.
 *
 * Frequency sweep shift register.
 */
void S1::write_reg0(uint8_t data) {
    if (!enabled) {
        return;
    }
    _reg0 = data;
    bool old_dir = _sweep_dir;
    _sweep_dir = (data & 0x8) != 0;
    _sweep_shift = data & 0x7;
    _sweep_period = (data >> 4) & 7;
    if (old_dir && !_sweep_dir && _last_sub) {
        _shift_ena = false;
        active = false;
    }
}

/**
 * @brief write register 4.
 *
 * Frequency register upper.
 *
 * When next step does not change Length counter, if previously disabled
 * and now enabled and counter != 0, decrement. If length == 0, disable
 * channel.
 *
 * When triggered if length == 0, set to max length.
 *
 * When triggering, and next does not change length counter, and length
 * is now enabled and length is being set to max length, decrement counter.
 */
void S1::write_reg4(uint8_t data, int next_length) {
    if (!enabled) {
        return;
    }
    bool prev_use_len = _use_len;
    bool trigger = ((data & 0x80) != 0);
    bool next_frame_length = (next_length & 1) != 0;
    _int_freq = (((uint16_t)(data & 7) << 8) & 0x700) | (_int_freq & 0xff);
    _sweep_freq = _int_freq;
    _use_len = (data & 0x40) != 0;
    start(trigger, prev_use_len, next_frame_length);
    if (trigger && active) {
        sample = ((int8_t)_wave[_pos] - 8) * _volume;
        _shift_ena = (_sweep_shift != 0) || (_sweep_period != 0);
        _sweep_clk = (_sweep_period != 0) ?_sweep_period : 8;
        _last_sub = false;
        if (_sweep_shift != 0 && next_freq() >= 2048) {
            _shift_ena = false;
            active = false;
        }
    }
}

/**
 * @brief write register 0.
 *
 * Frequency sweep shift register.
 */
void S3::write_reg0(uint8_t data) {
    if (!enabled) {
        return;
    }
    if ((data & 0x80) == 0) {
        active = false;
        _dac_enable = false;
        _chan_ena = false;
    } else {
        _dac_enable = true;
        _chan_ena = true;
    }
}

/**
 * @brief write register 1.
 *
 * Sound length register.
 */
void S3::write_reg1(uint8_t data) {
    if (!enabled) {
        return;
    }
    _length = _max_length - (data & 0xff);
}

/**
 * @brief write register 2.
 *
 * Output volume
 */
void S3::write_reg2(uint8_t data) {
    static const uint8_t vol_mul[4] = { 0, 0x10, 0x08, 0x04 };
    if (!enabled) {
        return;
    }
    _out_vol = (data >> 5) & 0x3;
    _int_vol = vol_mul[_out_vol];
}

/**
 * @brief read wave register.
 *
 * The wave register is store expanded to two samples, so
 * they have to be combined in read function.
 *
 * param[out] data   Value of wave at index.
 * param[in]  addr   Index to read.
 */
void S3::read_wave(uint8_t& data, uint16_t index) const {
     if (active) {
       uint8_t last_read = 0xff;
       if (_freq_cnt == 0x7ff) {
            int pos = (_pos) & 0x1e;
            last_read = (_wave[pos] << 4);
            last_read |= _wave[pos | 1];
       }
        data = last_read;
     } else {
         index <<= 1;
         data = _wave[index] << 4;
         data |= _wave[index | 1];
     }
}

/**
 * @brief write wave register.
 *
 * The wave register is store expanded to two samples, so
 * they have to be split apart and shifted for writing.
 *
 * param[out] data   Value of wave at index.
 * param[in]  addr   Index to read.
 */
void S3::write_wave(uint8_t data, uint16_t index) {
     if (active)
        return;
     index <<= 1;
     _wave[index] = (data >> 4) & 0xf;
     _wave[index | 1] = (data & 0xf);
}


/**
 * @brief reset channel.
 *
 * Stops any channel activity and set it to default state.
 */
void S4::reset() {
    active = false;
    write_reg0(0);
    write_reg2(0);
    write_reg3(0);
    write_reg4(0,0);
}

/**
 * @brief simulate the noise shift register.
 *
 * The shift register is either 15 or 7 bits long. We simulate this by a
 * 16 bit register. Each step the shift register is shifted 1 bit to the
 * left. The top two bits (15,14) or (7,6) are compared and fed into lowest
 * bit of shift register. Bit 15 or 7 is taken as the output.
 */
void S4::shift() {
    _shift_reg <<= 1;
    if (_steps) {
        _shift_reg |= ((_shift_reg >> 6) ^ (_shift_reg >> 7)) & 1;
        /* Copy output to upper bit */
        _shift_reg &= 0x7fff;
        _shift_reg |= (_shift_reg << 8) & 0x8000;
    } else {
        _shift_reg |= ((_shift_reg >> 14) ^ (_shift_reg >> 15)) & 1;
    }
    if (_dac_enable) {
        sample = ((_shift_reg & 0x8000) ? 7: -8)  * _volume;
    } else {
        sample = 0;
    }
}

/**
 * @brief Update frequency counter for one clock pulse.
 *
 * If the divide ratio reaches zero. We reset the divide ratio, then if
 * the polynomial counter clock reached zero we generate and output and
 * reset the counter clock to 2^n.
 */
void S4::cycle() {
    if (_delay) {
        _delay = false;
        return;
    }
    if (active) {
        if (_div == 0) {
            /* Do clocking */
            _div = _div_ratio;
            if (_clk_cnt == 0) {
                shift();
                _clk_cnt = (1 << _clk_freq) & 0x3fff;
            } else {
                _clk_cnt--;
            }
        } else {
            _div--;
        }
    } else {
        sample = 0;
    }
}


/**
 * @brief write register 1.
 *
 * Sound length register.
 */
void S4::write_reg1(uint8_t data) {
    _length = _max_length - (data & 0x3f);
}

/**
 * @brief write register 3.
 *
 * Divider register.
 */
void S4::write_reg3(uint8_t data) {
    if (!enabled) {
        return;
    }
    _reg3 = data;
    _clk_freq = (data >> 4) & 0xf;
    _steps = (data & 0x8) != 0;
    _div_ratio = (data & 07);
}

/**
 * @brief read register 4.
 *
 * Frequency register upper.
 */
void S4::write_reg4(uint8_t data, int next_length) {
    if (!enabled) {
        return;
    }
    bool prev_use_len = _use_len;
    bool trigger = ((data & 0x80) != 0);
    bool next_frame_length = (next_length & 1) != 0;
    _use_len = (data & 0x40) != 0;
    start(trigger, prev_use_len, next_frame_length);
    if (trigger && active) {
        _div = _div_ratio;
        _clk_cnt = (1 << _clk_freq) & 0x3fff;
    }
}



/**
 * @brief Called 512 times per second to update the sequencer.
 *
 * We need to call update length every other cycle. Every 4
 * cycles we call update sweep.
 *
 */
void Apu::cycle_sound() {
    if (!_enabled) {
        return;
    }
    switch(_fr_counter++) {
    case 2:
    case 6:    s1.update_sweep();
               /* Fall through */
               [[fallthrough]];
    case 0:
    case 4:    s1.update_length();
               s2.update_length();
               s3.update_length();
               s4.update_length();
               break;
    case 1:
    case 3:    /* Nothing to update */
               break;
    case 7:    s1.update_volume();
               s2.update_volume();
               s4.update_volume();
               _fr_counter = 0;
               break;
    }
}

void Apu::cycle() {
   if (_sample_cnt == 0) {
       /* Generate a audio sample */
       SO1 = SO2 = 0;
       if (regs[1] & 0x01) {
          SO1 += (s1.sample / 4);
       }
       if (regs[1] & 0x02) {
          SO1 += (s2.sample / 4);
       }
       if (regs[1] & 0x04) {
          SO1 += (s3.sample / 4);
       }
       if (regs[1] & 0x08) {
          SO1 += (s4.sample / 4);
       }
       if (regs[1] & 0x10) {
          SO2 += (s1.sample / 4);
       }
       if (regs[1] & 0x20) {
          SO2 += (s2.sample / 4);
       }
       if (regs[1] & 0x40) {
          SO2 += (s3.sample / 4);
       }
       if (regs[1] & 0x80) {
          SO2 += (s4.sample / 4);
       }
       /* Compute volume of samples */
       switch ((regs[0] >> 4) & 07) {
       case 0:   SO1 = 0; break;
       case 1:   SO1 = SO1 / 8; break;
       case 2:   SO1 = SO1 / 4; break;
       case 3:   SO1 = (SO1 /4) + (SO1 / 8); break;
       case 4:   SO1 = SO1 / 2; break;
       case 5:   SO1 = (SO1 / 2) + (SO1 / 4); break;
       case 6:   SO1 = (SO1 / 2) + (SO1 / 4) + (SO1 / 8); break;
       case 7:   break;
       }
       switch (regs[0] & 07) {
       case 0:   SO2 = 0; break;
       case 1:   SO2 = SO2 / 8; break;
       case 2:   SO2 = SO2 / 4; break;
       case 3:   SO2 = (SO2 /4) + (SO2 / 8); break;
       case 4:   SO2 = SO2 / 2; break;
       case 5:   SO2 = (SO2 / 2) + (SO2 / 4); break;
       case 6:   SO2 = (SO2 / 2) + (SO2 / 4) + (SO2 / 8); break;
       case 7:   break;
       }
       /* Output the sample */
       audio_output(SO1, SO2);
       _sample_cnt = 33;
   }
   _sample_cnt--;
   /* If not enabled, nothing left to do */
   if (!_enabled) {
       return;
   }
   s1.cycle();               /* Cycle channel clock */
   s2.cycle();               /* Cycle channel clock */
   s3.cycle();               /* Channel 2 is clocked 2 times speed */
   s4.cycle();               /* Cycle channel clock */
}

void Apu::read_reg(uint8_t &data, uint16_t addr) const {
    switch(addr & 0xff) {
    case 0x10:    /* NR 10 */
                  s1.read_reg0(data);
                  break;
    case 0x11:    /* NR 11 */
                  s1.read_reg1(data);
                  break;
    case 0x12:    /* NR 12 */
                  s1.read_reg2(data);
                  break;
    case 0x13:    /* NR 13 */
                  s1.read_reg3(data);
                  break;
    case 0x14:    /* NR 14 */
                  s1.read_reg4(data);
                  break;
    case 0x15:    /* NR 20 Unused */
                  s2.read_reg0(data);
                  break;
    case 0x16:    /* NR 21 */
                  s2.read_reg1(data);
                  break;
    case 0x17:    /* NR 22 */
                  s2.read_reg2(data);
                  break;
    case 0x18:    /* NR 23 */
                  s2.read_reg3(data);
                  break;
    case 0x19:    /* NR 24 */
                  s2.read_reg4(data);
                  break;
    case 0x1A:    /* NR 30 */
                  s3.read_reg0(data);
                  break;
    case 0x1B:    /* NR 31 */
                  s3.read_reg1(data);
                  break;
    case 0x1C:    /* NR 32 */
                  s3.read_reg2(data);
                  break;
    case 0x1D:    /* NR 33 */
                  s3.read_reg3(data);
                  break;
    case 0x1E:    /* NR 34 */
                  s3.read_reg4(data);
                  break;
    case 0x1F:    /* NR 40 Unused */
                  s4.read_reg0(data);
                  break;
    case 0x20:    /* NR 41 */
                  s4.read_reg1(data);
                  break;
    case 0x21:    /* NR 42 */
                  s4.read_reg2(data);
                  break;
    case 0x22:    /* NR 43 */
                  s4.read_reg3(data);
                  break;
    case 0x23:    /* NR 44 */
                  s4.read_reg4(data);
                  break;
    case 0x24:    /* NR 50 */
    case 0x25:    /* NR 51 */
                  data = regs[addr & 1];
                  break;
    case 0x26:    /* NR 52 */
                  data = (_enabled << 7) | 0x70;
                  data |= (s1.active << 0);
                  data |= (s2.active << 1);
                  data |= (s3.active << 2);
                  data |= (s4.active << 3);
                  break;
    case 0x27:    /* Unused */
    case 0x28:    /* Unused */
    case 0x29:    /* Unused */
    case 0x2A:    /* Unused */
    case 0x2B:    /* Unused */
    case 0x2C:    /* Unused */
    case 0x2D:    /* Unused */
    case 0x2E:    /* Unused */
    case 0x2F:    /* Unused */
                  data = 0xff;
                  break;
    case 0x30: case 0x31: case 0x32: case 0x33:
    case 0x34: case 0x35: case 0x36: case 0x37:
    case 0x38: case 0x39: case 0x3A: case 0x3B:
    case 0x3C: case 0x3D: case 0x3E: case 0x3F:
                 s3.read_wave(data, addr & 0xf);
                 break;
    }
// if (trace_flag) printf("Read %02x %02x\n", (addr & 0xff), data);
}

void Apu::write_reg(uint8_t data, uint16_t addr) {
//if (trace_flag) printf("Write %02x %02x\n", (addr & 0xff), data);
    switch(addr & 0xff) {
    case 0x10:    /* NR 10 */  /* Sound data */
                  s1.write_reg0(data);
                  break;
    case 0x11:    /* NR 11 */
                  s1.write_reg1(data);
                  break;
    case 0x12:    /* NR 12 */
                  s1.write_reg2(data);
                  break;
    case 0x13:    /* NR 13 */
                  s1.write_reg3(data);
                  break;
    case 0x14:    /* NR 14 */
                  s1.write_reg4(data, _fr_counter);
                  break;
    case 0x15:    /* NR 20  Unused */
                  s2.write_reg0(data);
                  break;
    case 0x16:    /* NR 21 */
                  s2.write_reg1(data);
                  break;
    case 0x17:    /* NR 22 */
                  s2.write_reg2(data);
                  break;
    case 0x18:    /* NR 23 */
                  s2.write_reg3(data);
                  break;
    case 0x19:    /* NR 24 */
                  s2.write_reg4(data, _fr_counter);
                  break;
    case 0x1A:    /* NR 30 */
                  s3.write_reg0(data);
                  break;
    case 0x1B:    /* NR 31 */
                  s3.write_reg1(data);
                  break;
    case 0x1C:    /* NR 32 */
                  s3.write_reg2(data);
                  break;
    case 0x1D:    /* NR 33 */
                  s3.write_reg3(data);
                  break;
    case 0x1E:    /* NR 34 */
                  s3.write_reg4(data, _fr_counter);
                  break;
    case 0x1F:    /* NR 40 - Unused */
                  s4.write_reg0(data);
                  break;
    case 0x20:    /* NR 41 */
                  s4.write_reg1(data);
                  break;
    case 0x21:    /* NR 42 */
                  s4.write_reg2(data);
                  break;
    case 0x22:    /* NR 43 */
                  s4.write_reg3(data);
                  break;
    case 0x23:    /* NR 44 */
                  s4.write_reg4(data, _fr_counter);
                  break;
    case 0x24:    /* NR 50 */
    case 0x25:    /* NR 51 */
                  if (_enabled) {
                      regs[addr&1] = data;
                  }
                  break;
    case 0x26:    /* NR 52 */
                  /* Check if changing state of enable flag. */
                  if (_enabled != ((data & 0x80) != 0)) {
                      _enabled = ((data & 0x80) != 0);
                      /* Add code to clear register if not enabled */
                      if (!_enabled) {
                         s1.reset();
                         s2.reset();
                         s3.reset();
                         s4.reset();
                         regs[0] = 0;
                         regs[1] = 0;
                         _fr_counter = 0;
                      }
                      s1.enabled = _enabled;
                      s2.enabled = _enabled;
                      s3.enabled = _enabled;
                      s4.enabled = _enabled;
                  }
                  break;
    case 0x27:    /* Unused */
    case 0x28:    /* Unused */
    case 0x29:    /* Unused */
    case 0x2A:    /* Unused */
    case 0x2B:    /* Unused */
    case 0x2C:    /* Unused */
    case 0x2D:    /* Unused */
    case 0x2E:    /* Unused */
    case 0x2F:    /* Unused */
                  break;
    case 0x30: case 0x31: case 0x32: case 0x33:
    case 0x34: case 0x35: case 0x36: case 0x37:
    case 0x38: case 0x39: case 0x3A: case 0x3B:
    case 0x3C: case 0x3D: case 0x3E: case 0x3F:
                 s3.write_wave(data, addr & 0xf);
                 break;
    }
}
