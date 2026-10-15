#ifndef XIAO_TEMPERATURE_CONFIG_H
#define XIAO_TEMPERATURE_CONFIG_H

#include <Arduino.h>
#include <RadioLib.h>
#include <string.h>
#include "secrets.h"

// Minimum interval between temperature uplinks; airtime rules can extend it.
constexpr uint32_t uplinkIntervalSeconds = 10UL * 60UL;

constexpr size_t lorawanKeySize = 16;
uint8_t appKey[lorawanKeySize];
uint8_t nwkKey[lorawanKeySize];

int8_t hexDigitValue(char digit) {
  if (digit >= '0' && digit <= '9') {
    return digit - '0';
  }
  if (digit >= 'A' && digit <= 'F') {
    return digit - 'A' + 10;
  }
  if (digit >= 'a' && digit <= 'f') {
    return digit - 'a' + 10;
  }
  return -1;
}

bool decodeLoRaWANKey(const char* hex, uint8_t* key) {
  if (strlen(hex) != lorawanKeySize * 2) {
    return false;
  }

  for (size_t i = 0; i < lorawanKeySize; i++) {
    int8_t high = hexDigitValue(hex[i * 2]);
    int8_t low = hexDigitValue(hex[i * 2 + 1]);
    if (high < 0 || low < 0) {
      return false;
    }
    key[i] = (uint8_t)((high << 4) | low);
  }

  return true;
}

// Switzerland: EU868.
const LoRaWANBand_t Region = EU868;
const uint8_t subBand = 0;

// Tested XIAO ESP32S3 + Wio-SX1262 B2B kit pins.
const int LORA_SCK    = 7;
const int LORA_MISO   = 8;
const int LORA_MOSI   = 9;
const int LORA_NSS    = 41;
const int LORA_DIO1   = 39;
const int LORA_RESET  = 42;
const int LORA_BUSY   = 40;
const int LORA_RF_SW  = 38;

// Module(NSS, DIO1, RESET, BUSY)
SX1262 radio = new Module(
  LORA_NSS,
  LORA_DIO1,
  LORA_RESET,
  LORA_BUSY
);

LoRaWANNode node(&radio, &Region, subBand);

// Persistent storage namespace.
// Do not change this or erase flash to troubleshoot joining:
// doing so would lose the stored nonce history.
const char* STORAGE_NAMESPACE = "xiao-lora2";

// Compact result-code decoder for RadioLib 6.6.0.
String stateDecode(int16_t state) {
  switch (state) {
    case RADIOLIB_ERR_NONE:
      return "ERR_NONE";

    case RADIOLIB_ERR_CHIP_NOT_FOUND:
      return "ERR_CHIP_NOT_FOUND";

    case RADIOLIB_ERR_NETWORK_NOT_JOINED:
      return "ERR_NETWORK_NOT_JOINED";

    case RADIOLIB_ERR_UPLINK_UNAVAILABLE:
      return "ERR_UPLINK_UNAVAILABLE";

    case RADIOLIB_ERR_NO_RX_WINDOW:
      return "ERR_NO_RX_WINDOW";

    case RADIOLIB_ERR_JOIN_NONCE_INVALID:
      return "ERR_JOIN_NONCE_INVALID";

    case RADIOLIB_ERR_CHECKSUM_MISMATCH:
      return "ERR_CHECKSUM_MISMATCH";

    case RADIOLIB_LORAWAN_NO_DOWNLINK:
      return "LORAWAN_NO_DOWNLINK";

    case RADIOLIB_LORAWAN_NEW_SESSION:
      return "LORAWAN_NEW_SESSION";

    case RADIOLIB_LORAWAN_SESSION_RESTORED:
      return "LORAWAN_SESSION_RESTORED";

    case RADIOLIB_LORAWAN_NONCES_DISCARDED:
      return "LORAWAN_NONCES_DISCARDED";

    default:
      return String("RadioLib code ") + String(state);
  }
}

#endif