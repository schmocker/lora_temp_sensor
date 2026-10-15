#ifndef XIAO_TEMPERATURE_SECRETS_H
#define XIAO_TEMPERATURE_SECRETS_H

#include <Arduino.h>

// Copy this file to secrets.h and enter the same values as your TTN device.
// Keep secrets.h out of version control.
uint64_t joinEUI = 0x0000000000000000;
uint64_t devEUI = 0x0000000000000000;

const char appKeyHex[] = "00000000000000000000000000000000";
const char nwkKeyHex[] = "00000000000000000000000000000000";

#endif
