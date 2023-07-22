/*
 * GB - Audio Processing Unit
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
#include <cstdio>
#include <cstdlib>
#include <ctime>
//#include "Device.h"
#include "System.h"

/**
 * @brief Default Sound channel object.
 *
 * @class Sound Apu.h "Apu.h"
 *
 * All Game Boy sound channels have the same basic functions.
 * They start with a frequency divider (or noise generator for 
 * channel 4). Whenever this divider counts down to zero the next
 * sample is generated, the counter is also reloaded with the 
 * initial frequency setting. The counter is clocked off either the
 * dot clock / 2 or by 4.
 *
 * Each channel also has a length counter which is used for decremented
 * at a rate of 256hz. When this counter reaches zero audio output stops
 * If the sound generator is started without using a lenght the counter
 * is loaded with the max value.
 *
 * Each channel also has a volume envolope generator. This will either
 * increase of decrease the volume at a rate of 64hz. When the volume
 * reaches 15 or 0 the envolope stops operating.
 *
 * Channel 1 has a frequency sweep function which adjusts the frequency
 * counter at a rate of 128hz. The frequency can be shifted up or down
 * by a fractional amount each frame.
 */
class Sound {
protected:
     uint8_t _wave[32];   /**< Pointer to wave data */
     int     _freq;       /**< Frequency of signal */
     int     _int_freq;   /**< Initial frequency to reload counter */
     int     _count;      /**< Current frequencey counter */
     int     _length;     /**< Length in 1/64th of a second */
     int     _int_len;    /**< Initial value of length */
     int     _use_len;    /**< Use length value */
     int     _pos;        /**< Current position in playback */
     int     _volume;     /**< Current output volume */
     int     _int_vol;    /**< Initial volume */
     int     _int_vol_len;/**< Initial volume length */
     int     _vol_sweep;  /**< Volume sweep */
     int     _vol_dir;    /**< Sweep direction */
     int     _vol_len;    /**< Length of volume envolope */
     bool    _vol_env;    /**< Do volume envolope */
     int     _duty;       /**< Duty cycle */
     int     _freq_cnt;   /**< Frequence sweep counter */
     uint8_t _regs[5];    /**< Copy of register data */
     uint8_t _chan;       /**< Channel number */
     bool    _dac_enable; /**< Channel's DAC is disabled */

     const static uint8_t sq_wave[32];
     const static uint8_t int_wave[32];
public:
     bool    enabled;     /**< Channel enabled */
     bool    active;      /**< Sound currently playing */
     uint8_t sample;      /**< Current output value */

     Sound() : enabled(false), active(false), sample(0) {
          _freq = _int_freq = _count = _length = _int_len = 0;
          _use_len = _pos = _volume = _int_vol = _int_vol_len = 0;
          _vol_sweep = _vol_dir = _vol_len = _duty = _freq_cnt = 0;
          _chan = 0; _dac_enable = true;
          _vol_env = false;
          for (int r = 0; r < 5; r++) {
              _regs[r] = 0;
          }
          for (int i = 0; i < 32; i++) {
              _wave[i] = sq_wave[i];
          }
     }

     /**
      * @brief Start playing a sound.
      *
      * Set initial frequency, length and volume.
      */
      virtual void start(bool master_enable, bool prev_use_len, bool next_length) {
          if (!_dac_enable) {
              return;
          }
         
          /* Set initial frequency and length to play */
          _freq = 2048 - _int_freq;
         
         
          if (_length == 0) {
             _length = max_length();
          }
          _volume = _int_vol;
          if (_int_vol_len != 0) {
              _vol_len = _int_vol_len;
              _vol_env = true;
          }
          _freq_cnt = 0;
          active = master_enable;
if (trace_flag) printf("Start play:%d %d ul=%d vl=%d v=%d en=%d\n", _chan, _freq, _use_len, _vol_len, _volume, _dac_enable);
     }

     /**
      * @brief reset channel.
      *
      * Stops any channel activity and set it to default state.
      */
     virtual void reset() {
if (trace_flag) printf("Channel reset\n");
         active = false;
         write_reg0(0);
         write_reg1(0);
         write_reg2(0);
         write_reg3(0);
         write_reg4(0,false,false);
     }

     /**
      * @brief Update channels length counter.
      */
     virtual void update_length() {
if (trace_flag) printf("CH %d Len %d Active %d\n", _chan, _length, active);
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
     virtual void update_volume() {
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
                      _volume = new_vol;
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
     virtual void update_sweep() {
     }

     /**
      * @brief Update frequence counter for one clock pulse.
      */
     virtual void cycle() {
         if (active) {
             _freq--;
             if (_freq < 0) {
                 _freq = 2048 - _int_freq;
                 _pos++;
                 if ((_pos & 0x7) == 0) {
                     _pos = _duty << 3;
                 }
                 if (_dac_enable) {
                     sample = _wave[_pos] * _volume;
                 } else {
                     sample = 0;
                 }
             }
         } else {
             sample = 0;
         }
     }

     /**
      * @brief return maximum length for sound.
      *
      * default length is 64 1/256th of a second.
      * @return 64
      */
     virtual int max_length() const {
         return 64;
     }

     /**
      * @brief read register 0.
      *
      * Frequency sweep shift register.
      */
     virtual void read_reg0(uint8_t& data) const {
         data = _regs[0] | 0x80;
     }

     /**
      * @brief read register 1.
      *
      * Sound length register.
      */
     virtual void read_reg1(uint8_t& data) const {
         data = _regs[1] | 0x3f;
     }

     /**
      * @brief read register 2.
      *
      * Length envolope register.
      */
     virtual void read_reg2(uint8_t& data) const {
         data = _regs[2];
     }

     /**
      * @brief read register 3.
      *
      * Frequency register lower.
      */
     virtual void read_reg3(uint8_t& data) const {
         data = 0xff;
     }

     /**
      * @brief read register 4.
      *
      * Frequency register upper.
      */
     virtual void read_reg4(uint8_t& data) const {
         data = _regs[4] | 0xbf;
     }

     /**
      * @brief write register 0.
      *
      * Frequency sweep shift register.
      */
     virtual void write_reg0(uint8_t data) {
         if (!enabled) {
             return;
         }
         _regs[0] = data;
     }

     /**
      * @brief write register 1.
      *
      * Sound length register.
      */
     virtual void write_reg1(uint8_t data) {
         if (!enabled) {
             return;
         }
         _regs[1] = data;
         _length = max_length() - (data & 0x3f);
         _duty = (data >> 6) & 3;
     }

     /**
      * @brief write register 2.
      *
      * Volume envolope register.
      */
     virtual void write_reg2(uint8_t data) {
         if (!enabled) {
             return;
         }
         _regs[2] = data;
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
     virtual void write_reg3(uint8_t data) {
         if (!enabled) {
             return;
         }
         _regs[3] = data;
         _int_freq = (_int_freq & 0x700) | ((int)data & 0xff);
     }

     /**
      * @brief write register 4.
      *
      * Frequency register upper.
      */
     virtual void write_reg4(uint8_t data, bool master_enable, bool next_length) {
         if (!enabled) {
             return;
         }
         _regs[4] = data;
         bool prev_use_len = _use_len;
         _int_freq = (((int)(data & 7) << 8) & 0x700) | (_int_freq & 0xff);
         _use_len = (data & 0x40) != 0;

         if ((data & 0x80) != 0) {
             /* Check if we should trigger a length adjust */
             if (!prev_use_len && _use_len && next_length && _length > 0) {
                _length--;
             }
             start(master_enable, prev_use_len, next_length);
             _pos = _duty << 3;
         } else {
             if (!prev_use_len && _use_len && !next_length && _length > 0) {
                 _length --;
                 if (_length == 0) {
                     active = false;
                     sample = 0;
                 }
            }
         }
     }

};

/**
 * @brief Sound Channel 2.
 *
 * @class S2 Apu.h "Apu.h"
 *
 * Sound channel 2 is the default channel. It plays a fixed wave of
 * 8 samples. These are set to play a square wave. It lacks the 
 * Frequency sweep function.
 */
class S2 : public Sound {
public:
     S2() {
          /* Initialize wave table */
          _chan = 2;
     }

};

/**
 * @brief Sound Channel 1.
 *
 * @class S1 Apu.h "Apu.h"
 *
 * Sound channel 1 is the same as Sound Channel 2, but includes
 * a frequency sweep function. It also plays an 8 samples. These
 * are set to play a square wave.
 */
class S1 : public S2 {
     int     _sweep_period; /**< Frequency sweep interval */
     int     _sweep_clk;    /**< Frequency sweep clock */
     bool    _sweep_dir;   /**< Direction of frequence sweep */
     int     _sweep_shift; /**< Number of steps */
     int     _sweep_freq; /**< Copy of frequency to update */
     bool    _shift_ena;  /**< Enable frequency shift */
     bool    _last_sub;   /**< Last update was subtract */
public:

     S1() : S2() {
          _chan = 1;
          _sweep_period = _sweep_shift = _sweep_freq = _sweep_clk = 0;
          _sweep_dir = _shift_ena = _last_sub = false;
     }

     /**
      * @brief Update frequence counter for one clock pulse.
      */
     virtual void cycle() override {
         if (active) {
             _freq--;
             if (_freq < 0) {
                 _freq = 2048 - _sweep_freq;
                 _pos++;
                 if ((_pos & 0x7) == 0) {
                     _pos = _duty << 3;
                 }
                 if (_dac_enable) {
                     sample = _wave[_pos] * _volume;
                 } else {
                     sample = 0;
                 }
             }
         } else {
             sample = 0;
         }
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
     void update_step() {
         int new_freq = next_freq();
if (trace_flag) printf("Update freq %d(%04x) to %d(%04x)\n", _int_freq, _int_freq, new_freq, new_freq);
         if (new_freq >= 2048) {
if (trace_flag) printf("Overflow\n");
             active = false;
             _shift_ena = false;
             sample = 0;
         } else { 
             if (_sweep_shift) {
                 _sweep_freq = new_freq;
             }
             if (next_freq() > 2048) {
                 active = false;
                 _shift_ena = false;
                 sample = 0;
             }
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
     uint16_t next_freq() {
         int new_freq = _sweep_freq >> _sweep_shift;
         if (_sweep_dir) {
              new_freq = _sweep_freq - new_freq;
              _last_sub = true;
         } else {
              new_freq = _sweep_freq + new_freq;
         }
         return new_freq;
     }
 

     /**
      * @brief Update channels frequency  sweep.
      */
     virtual void update_sweep() override {
         if (active && _shift_ena) {
              if (_sweep_clk > 0) {
                  _sweep_clk--;
              }
if (trace_flag) printf("Freq sweep %d %02x\n", _sweep_clk, _regs[0]);
             if (_sweep_clk == 0) {
                 if (_sweep_period != 0) {
                     _sweep_clk = _sweep_period;
                     update_step();
if (trace_flag) printf("Freq sweep new %d %02x\n", _sweep_clk, _regs[0]);
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
     virtual void write_reg0(uint8_t data) override {
         if (!enabled) {
             return;
         }
         _regs[0] = data;
         bool new_dir = (data & 0x8) != 0;
         _sweep_shift = data & 0x7;
         _sweep_period = (data >> 4) & 7;
         if (_sweep_dir && !new_dir && _last_sub) {
             _shift_ena = false;
             active = false;
         }
         _sweep_dir = new_dir;
     }

     /**
      * @brief write register 4.
      *
      * Frequency register upper.
      */
     virtual void write_reg4(uint8_t data, bool master_enable, bool next_length) override {
         if (!enabled) {
             return;
         }
         _regs[4] = data;
         bool prev_len = _use_len;
         _int_freq = (((int)(data & 7) << 8) & 0x700) | (_int_freq & 0xff);
         if (!active) {
         _sweep_freq = _int_freq;
         }
         _use_len = (data & 0x40) != 0;
         if ((data & 0x80) != 0) {
             start(master_enable, prev_len, next_length);
             _pos = _duty << 3;
             if (_sweep_shift != 0 || _sweep_period != 0) {
                 _last_sub = false;
                 _sweep_clk = _sweep_period;
                 _shift_ena = true;
                  if (_sweep_shift != 0 && next_freq() > 2048) {
                     _shift_ena = false;
                     active = false;
                }
             } else {
//                 if (_freq_sweep != 0) {
 //                   _shift_ena = true;
  //                  _freq_cnt = _freq_sweep;
   //              } else {
                    _shift_ena = false;
    //             }
             } //else if (_freq_cnt != 0) {
                 //_shift_ena = true;
             //}
         } else {
             if (!prev_len && _use_len && !next_length && _length > 0) {
                 _length --;
                 if (_length == 0) {
                     active = false;
                     sample = 0;
                 }
            }
         }
     }

};

/**
 * @brief Sound Channel 3
 *
 * @class S3 Apu.h "Apu.h"
 *
 * Sound Channel 3 is the same as sound channel 2, only it
 * plays a 32 sample wave which can be loaded by the CPU. It also
 * lacks a volume envolope.
 */
class S3 : public Sound {
     bool     _chan_ena;    /**< Indicates if channel is enabled or not */
     bool     _read_wave;   /**< Last cycle read the wave register */
     bool     _last_read; 
public:
     S3() {
          _chan = 3;
          _chan_ena = false;
          _read_wave = _last_read = false;
          /* Initialize wave table */
          for (int i = 0; i < 32; i++) {
              _wave[i] = int_wave[i];
          }
     }

     /**
      * @brief Update channels volume sweep.
      */
     virtual void update_volume() override {
     }

     /**
      * @brief Update frequence counter for one clock pulse.
      */
     virtual void cycle() override {
         if (active) {
             _freq--;
             _last_read = _read_wave;
             _read_wave = false;
if (trace_flag) printf("Wave freq %d\n",_freq);
             if (_freq <= 0) {
                 _freq = 2048 - _int_freq;
                 _pos++;
                 if (_pos == 32) {
                     _pos = 0;
                 }
                 if (_dac_enable) {
                     sample = _wave[_pos];
                     _last_read = false;
                     _read_wave = true;
                 } else {
                     sample = 0;
                 }
if (trace_flag) printf("Wave %02x %02x\n", _pos, sample);
                 switch(_volume) {
                 case 0: sample = 0; break;
                 case 1: break;
                 case 2: sample >>= 1; break;
                 case 3: sample >>= 2; break;
                 }
             }
         } else {
             sample = 0;
         }
     }


     /**
      * @brief return maximum length for sound.
      *
      * default length is 256 1/256th of a second.
      * @return 64
      */
     virtual int max_length() const override {
         return 256;
     }

     /**
      * @brief read register 0.
      *
      * Frequency sweep shift register.
      */
     virtual void read_reg0(uint8_t& data) const override {
         data = _regs[0] | 0x7f;
     }

     /**
      * @brief read register 1.
      *
      * Sound length register.
      */
     virtual void read_reg1(uint8_t& data) const override {
         data = 0xff;
     }

     /**
      * @brief read register 2.
      *
      * Length envolope register.
      */
     virtual void read_reg2(uint8_t& data) const override {
         data = _regs[2] | 0x9f;
     }

     /**
      * @brief write register 0.
      *
      * Frequency sweep shift register.
      */
     virtual void write_reg0(uint8_t data) override {
         if (!enabled) {
             return;
         }
         _regs[0] = data;
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
     virtual void write_reg1(uint8_t data) override {
         if (!enabled) {
             return;
         }
         _length = max_length() - data;
     }

     /**
      * @brief write register 2.
      *
      * Length envolope register.
      */
     virtual void write_reg2(uint8_t data) override {
         if (!enabled) {
             return;
         }
         _regs[2] = data;
         _int_vol = (data >> 5) & 0x3;
     }

     /**
      * @brief read register 4.
      *
      * Frequency register upper.
      */
     virtual void write_reg4(uint8_t data, bool master_enable, bool next_length) override {
         if (!enabled) {
             return;
         }
         _regs[4] = data;
         bool prev_len = _use_len;
         _int_freq = (((int)(data & 7) << 8) & 0x700) |
                       (_int_freq & 0xff);
         _use_len = (data & 0x40) != 0;
         if ((data & 0x80) != 0) {
             start(master_enable, prev_len, next_length);
             sample = _wave[_pos];
             _pos = 0;
         } else {
             if (!prev_len && _use_len && !next_length && _length > 0) {
                 _length --;
                 if (_length == 0) {
                     active = false;
                     sample = 0;
                 }
            }
         }
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
      void read_wave(uint8_t& data, uint16_t index) const {
           if (active) {
              if (_last_read) {
                  int pos = (_pos - 1) & 0x1e;
                  data = _wave[pos];
                  data |= _wave[pos | 1] >> 4;
if (trace_flag)     printf("Read wave %d %x\n", _pos, pos);
              } else {
                  data = 0xff;
              }
           } else {
               index <<= 1;
               data = _wave[index];
               data |= _wave[index | 1] >> 4;
           }
      }

     /**
      * @brief write wave register.
      *
      * The wave register is store expanded to two samples, so
      * they have to be split appart and shifted for writing.
      *
      * param[out] data   Value of wave at index.
      * param[in]  addr   Index to read.
      */
      void write_wave(uint8_t data, uint16_t index) {
           if (active)
              return;
           index <<= 1;
           _wave[index] = data & 0xf0;
           _wave[index | 1] = (data & 0xf) << 4;
      }
};

/**
 * @ brief Sound Channel 4
 *
 * @class S4 Apu.h "Apu.h"
 *
 * Sound Channel 4 lacks a frequency generator or a wave table. It
 * outputs a psuedo random stream to simulate noise (LSFR). Instead of a 
 * frequency divider, channel 4 has a simple divider the controls
 * when the LSFR should generate a new sample. The LSFR is a shift
 * register that is initialized to a random value, and each time it is
 * clocked shifts one bit right. The two high order bits are exclusive
 * or'd together and sent back into bit 0. The LSFR can be configured
 * as either 15 or 7 bits. It is implemented as a 16 bit register. The
 * output is always the high bit, and the highest two bits are feed
 * back into the shift register.
 */
class S4 : public Sound {
     uint16_t        _shift_reg;   /**< Random noise register */
     int             _div_ratio;   /**< Divider ratio */
     int             _steps;       /**< 7 or 15 bit LSFR */
     int             _clk_freq;    /**< Clock frequency */
     int             _div;         /**< Freq divide counter */
     uint16_t        _clk_cnt;     /**< Shift counter */
public:

     S4() {
          _chan = 4;
          _div_ratio = _steps = _clk_freq = _div = 0;
          _clk_cnt = 0;
          std::srand(std::time(nullptr));
          int seed = std::rand();

          _shift_reg = (seed >> 10) & 0x7fff;
     }

     void shift() {
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
             sample = ((_shift_reg & 0x8000) ? 0x10: 0x00) * _volume;
         } else {
             sample = 0;
         }
     }

     virtual void cycle() override {
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
     }

     /**
      * @brief read register 0.
      *
      * Frequency sweep shift register.
      */
     virtual void read_reg0(uint8_t& data) const override {
         data = 0xff;
     }

     /**
      * @brief read register 1.
      *
      * Sound length register.
      */
     virtual void read_reg1(uint8_t& data) const override {
         data = 0xff;
     }

     /**
      * @brief read register 3.
      *
      * Frequency register lower.
      */
     virtual void read_reg3(uint8_t& data) const override {
         data = _regs[3];
     }

     /**
      * @brief write register 3.
      *
      * Divider register.
      */
     virtual void write_reg3(uint8_t data) override {
         if (!enabled) {
             return;
         }
         _regs[3] = data;
         _clk_freq = (data >> 4) & 0xf;
         _steps = (data & 0x8) != 0;
         _div_ratio = (data & 07);
     }

     /**
      * @brief read register 4.
      *
      * Frequency register upper.
      */
     virtual void write_reg4(uint8_t data, bool master_enable, bool next_length) override {
         if (!enabled) {
             return;
         }
         _regs[4] = data;
         bool prev_len = _use_len;
         _use_len = (data & 0x40) != 0;
         if ((data & 0x80) != 0) {
             start(master_enable, prev_len, next_length);
             _div = _div_ratio;
             _clk_cnt = (1 << _clk_freq) & 0x3fff;
         } else {
             if (!prev_len && _use_len && !next_length && _length > 0) {
                 _length --;
                 if (_length == 0) {
                     active = false;
                     sample = 0;
                 }
            }
         }
     }
};

/**
 * @brief Audio Processing Unit.
 *
 * @class Apu Apu.h "Apu.h"
 *
 * The audio processing unit generates all the sounds for the
 * Game Boy. It is clocked off the main system clock and from
 * the DIV clock. If the DIV register is written this can change
 * how timing will occur.
 *
 * @verbatim
 * NR10    Bit 7        not used.
 *         Bit 6-4      Sweep time ts.
 *            000:  Sweep off.
 *            001:  ts=1/f128 =  7.8ms   8176
 *            010:  ts=2/f128 = 15.6ms  16352
 *            011:  ts=3/f128 = 23.4ms  24528
 *            100:  ts=4/f128 = 31.3ms  32809
 *            101:  ts=5/f128 = 39.1ms  40986
 *            110:  ts=6/f128 = 46.9ms  49161
 *            111:  ts=7/f128 = 54.7ms  57337
 *         Bit 3        Sweep Increase/Decrease.
 *         Bit 2-0      Sweep Shift Number 0-7
 *
 * NR11    Bit 7-6      Bit 7 output, 6 Write only
 *             00:   12.5%
 *             01:   25%
 *             10:   50%
 *             11:   75%
 *         Bit 5-0:     Sound length t1(0 to 63).
 *
 * NR12    Bit 7-4      Default Envelope Value
 *         Bit 3        Envelop Down/Up
 *         Bit 2-0      Length of step n(0 to 7).
 *                          Nx1/64 sec.
 *
 * NR13    Bit 7-0      Low order freq (Write only)
 *
 * NR14    Bit 7        Initialize. (Write only)
 *         Bit 6        Counter Continuous Selection
 *         Bit 5-3      Unused.
 *         Bit 2-0      High order freq (Write only)
 *
 *         f = 4194304/(4x2x(2048-X)) HZ.
 *               Min = 64Hz, 131.1kHz
 *
 * NR21-24  Same as NR11-NR14
 *
 * NR30    Bit 7      Stop/Enable sound. (R/W)
 *         Bit 6-0    Unused
 *
 * NR31    Bit 7-3     Length of sound (0-255)
 *         Bit 2-0     Unused.
 (                 Length = (256-t1)x(1/256) sec
 *
 * NR32    Bit 7      unused.
 *         Bit 6-5    Output level.
 *              00     Mute
 *              01     4 bit.
 *              10     shift left 1. 1/2
 *              11     shift left 2. 1/4
 * NR33    Bit 7-0     Low order freq (write only)
 *
 * NR34    Bit 7       Initialize.
 *         Bit 6       Counter/continuous. 
 *                     0 continuous.
 *                     1 length NR31. When done bit2 NR52 Sound 3 on reset.
 *
 * NR41    Bit 7-6     Unused.
 *         Bit 5-0     Sound length t1(0 to 63).
 *
 * NR42    Same NR12
 *
 * NR43     Bit 7-4    Selects Shift Clock.
 *              000:   fx1/2^3x2
 *              001:   fx1/2^3x1
 *              010:   fx1/2^3x1/2
 *              011:   fx1/2^2x1/3
 *              100:   fx1/2^2x1/4
 *              101:   fx1/2^2x1/5
 *              110:   fx1/2^2x1/6
 *              111:   fx1/2^2x1/7
 *          Bit 3      Selects number of polynomal 0=15,1=7
 *          Bit 2-0    Division ratio frequence.
 *
 * NR44     Bit 7      Initialize.
 *          Bit 6      Counter/Continuous.
 *
 * NR50     Bit 7      Vin -> S02 on/off
 *          Bit 6-4    SO2 output level 0-7
 *          Bit 3      Vin -> S01 on/off
 *          Bit 2-0    SO1 output level 0-7
 *
 * NR51     Bit 7-4    Output Sound 4-1 to SO2
 *          Bit 3-0    Output Sound 4-1 to S01
 *
 * NR52     Bit 7      Enable sounds.
 *          Bit 3-0    Sound 4-1 on flag.
 *
 * @endverbatim
 *
 */
class Apu {
     uint8_t     *_irq_flg;        /**< Interrupt pointer */

     uint8_t      regs[2];         /**< NR5x registers */
     int          _fr_counter;     /**< Frame counter */
     int          _sample_cnt;     /**< Sample counter */
     bool         _enabled;        /**< Sound system enabled */

     S1           s1;              /**< Sound channel 1 */
     S2           s2;              /**< Sound channel 2 */
     S3           s3;              /**< Sound channel 3 */
     S4           s4;              /**< Sound channel 4 */

     uint8_t      SO1;             /**< Sound output S1 Right */
     uint8_t      SO2;             /**< Sound output S2 Left */
public:
     Apu() {
        _fr_counter = 0;
        _sample_cnt = 0;
        _enabled = false;
        _irq_flg = NULL;
        regs[0] = 0;
        regs[1] = 0;
        SO1 = SO2 = 0;
     }

     /**
      * @brief Connect to interrupt register
      *
      * Set pointer to where interrupts need to be updated.
      *
      * @param irq_flg Pointer to interrupt register in CPU.
      */
     void set_irq(uint8_t *irq_flg) {
          _irq_flg = irq_flg;
     }

    /**
     * @brief Process a single cycle, early.
     *
     */
    void cycle_early() {
       s3.cycle();               
    }

    /**
     * @brief Process a single cycle.
     *
     * Every 32 cycles sample the output of each channel. Then call
     * each channels cycle function to update their frequency count.
     */
    void cycle() {
       _sample_cnt++;
       if (_sample_cnt == 32) {
           /* Generate a audio sample */
           SO1 = SO2 = 0;
           if (regs[1] & 0x01) {
              SO1 += (s1.sample >> 2);
           }
           if (regs[1] & 0x02) {
              SO1 += (s2.sample >> 2);
           }
           if (regs[1] & 0x04) {
              SO1 += (s3.sample >> 2);
           }
           if (regs[1] & 0x08) {
              SO1 += (s4.sample >> 2);
           }
           if (regs[1] & 0x10) {
              SO2 += (s1.sample >> 2);
           }
           if (regs[1] & 0x20) {
              SO2 += (s2.sample >> 2);
           }
           if (regs[1] & 0x40) {
              SO2 += (s3.sample >> 2);
           }
           if (regs[1] & 0x80) {
              SO2 += (s4.sample >> 2);
           }
           /* Compute volume of samples */
           switch ((regs[0] >> 4) & 07) {
           case 0:   SO1 = 0; break;
           case 1:   SO1 = SO1 >> 8; break;
           case 2:   SO1 = SO1 >> 4; break;
           case 3:   SO1 = (SO1 >> 4) + (SO1 >> 8); break;
           case 4:   SO1 = SO1 >> 2; break;
           case 5:   SO1 = (SO1 >> 2) + (SO1 >> 4); break;
           case 6:   SO1 = (SO1 >> 2) + (SO1 >> 4) + (SO1 >> 8); break;
           case 7:   break;
           }
           switch (regs[0] & 07) {
           case 0:   SO1 = 0; break;
           case 1:   SO1 = SO1 >> 8; break;
           case 2:   SO1 = SO1 >> 4; break;
           case 3:   SO1 = (SO1 >> 4) + (SO1 >> 8); break;
           case 4:   SO1 = SO1 >> 2; break;
           case 5:   SO1 = (SO1 >> 2) + (SO1 >> 4); break;
           case 6:   SO1 = (SO1 >> 2) + (SO1 >> 4) + (SO1 >> 8); break;
           case 7:   break;
           }
           /* Output the sample */
           audio_output(SO1, SO2);
           _sample_cnt = 0;
       }
       /* If not enabled, nothing left to do */
       if (!_enabled) {
           return;
       }
       s1.cycle();               /* Cycle channel clock */
       s2.cycle();               /* Cycle channel clock */
       s3.cycle();               /* Channel 2 is clocked 2 times speed */
       s4.cycle();               /* Cycle channel clock */
    }

    /**
     * @brief Called 512 times per second to update the sequencer.
     *
     * We need to call update length every other cycle. Every 4
     * cycles we call update sweep. 
     *
     */
    void cycle_sound() {
        if (!_enabled) {
            return;
        }
if (trace_flag) printf("Counter %d\n", _fr_counter);
        switch(_fr_counter++) {
        case 2:
        case 6:    s1.update_sweep();
                   /* Fall through */
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

    /**
     * @brief Audio Processor Unit register read.
     *
     * Read chanel registers. Each channel implements 
     * an individual read for each register. By use
     * of virtual functions channels can return the
     * correct value along with any stuck bits.
     */
    void read(uint8_t &data, uint16_t addr) const {
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
 if (trace_flag) printf("Read %02x %02x\n", (addr & 0xff), data);
    }

    /**
     * @brief Audio Processor Unit register write.
     *
     * Write chanel registers. Each channel implements 
     * an individual write for each register. By use
     * of virtual functions channels can update internal
     * values as they need to.
     */
    void write(uint8_t data, uint16_t addr) {
if (trace_flag) printf("Write %02x %02x\n", (addr & 0xff), data);
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
                      s1.write_reg4(data, _enabled, _fr_counter & 1);
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
                      s2.write_reg4(data, _enabled, _fr_counter & 1);
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
                      s3.write_reg4(data, _enabled, _fr_counter & 1);
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
                      s4.write_reg4(data, _enabled, _fr_counter & 1);
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
};
