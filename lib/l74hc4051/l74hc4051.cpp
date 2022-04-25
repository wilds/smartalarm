/*
l74hc4051 lib 0x01

copyright (c) Davide Gironi, 2017

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/

#include "l74hc4051.h"

/**
 * Constructor
 */
L74HC4051::L74HC4051() { }

/**
 * init the shift register
 * @param s0pin pin attached to S0
 * @param s1pin pin attached to S1
 * @param s2pin pin attached to S2
 */
void L74HC4051::init(uint8_t s0pin, uint8_t s1pin, uint8_t s2pin) {
  _s0pin = s0pin;
  _s1pin = s1pin;
  _s2pin = s2pin;
  //output

  if (_s0pin >= 0)
    pinMode(_s0pin, OUTPUT);
  if (_s1pin >= 0)
    pinMode(_s1pin, OUTPUT);
  if (_s2pin >= 0)
    pinMode(_s2pin, OUTPUT);

  //low
  digitalWriteAllPins(LOW, LOW, LOW);
}

/**
 * Set a channel
 * @param channel number of the channel to read 0..7
 */
void L74HC4051::setChannel(uint8_t channel) {
  switch(channel) {
    case 0:
      digitalWriteAllPins(LOW, LOW, LOW);
      break;
    case 1:
      digitalWriteAllPins(HIGH, LOW, LOW);
      break;
    case 2:
      digitalWriteAllPins(LOW, HIGH, LOW);
      break;
    case 3:
      digitalWriteAllPins(HIGH, HIGH, LOW);
      break;
    case 4:
      digitalWriteAllPins(LOW, LOW, HIGH);
      break;
    case 5:
      digitalWriteAllPins(HIGH, LOW, HIGH);
      break;
    case 6:
      digitalWriteAllPins(LOW, HIGH, HIGH);
      break;
    case 7:
      digitalWriteAllPins(HIGH, HIGH, HIGH);
      break;
    default:
      break;
  }
}

void L74HC4051::digitalWriteAllPins(uint8_t s0val, uint8_t s1val, uint8_t s2val) {
  if (_s0pin >= 0)
    digitalWrite(_s0pin, s0val);
  if (_s1pin >= 0)
    digitalWrite(_s1pin, s1val);
  if (_s2pin >= 0)
    digitalWrite(_s2pin, s2val);
}