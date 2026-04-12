#pragma once
#include <Arduino.h>

#define IR_PIN D2
#define IR_ADDRESS 0xA002

#define IR_POWER 0x80
#define IR_INPUT 0x08
#define IR_MUTE 0xEA
#define IR_LEVEL 0x0A
#define IR_PLUS 0xAA
#define IR_EFFECT 0x0E
#define IR_MINUS 0x6A
#define IR_INPUT1 0x04
#define IR_INPUT2 0x82
#define IR_INPUT3 0x0C
#define IR_INPUT4 0x8C
#define IR_INPUT5 0x02
#define IR_AUX 0x42
#define IR_TEST 0x01

namespace z906remote::infrared {
    void enableIR();
    void handleIRCommand(uint32_t);
    void processIR();
} // namespace z906remote::infrared
