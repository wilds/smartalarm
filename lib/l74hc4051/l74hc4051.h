/*
l74hc4051 lib 0x01

copyright (c) Davide Gironi, 2017

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/

#include <Arduino.h>

#ifndef L74HC4051_H_
#define L74HC4051_H_

//max channels
#define L74HC4051_MAXCH 8

class L74HC4051 {
  public:
    L74HC4051();
    void init(uint8_t s0pin, uint8_t s1pin, uint8_t s2pin);
    void setChannel(uint8_t channel);
  private:
    uint8_t _s0pin = 0;
    uint8_t _s1pin = 0;
    uint8_t _s2pin = 0;
    void digitalWriteAllPins(uint8_t s0val, uint8_t s1val, uint8_t s2val);
};

#endif
