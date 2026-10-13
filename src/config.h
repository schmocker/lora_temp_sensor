#ifndef XIAO_TEMPERATURE_CONFIG_H
#define XIAO_TEMPERATURE_CONFIG_H

#include <Arduino.h>
#include <RadioLib.h>

// Minimum interval between temperature uplinks; airtime rules can extend it.
constexpr uint32_t uplinkIntervalSeconds = 10UL * 60UL;

// OTAA credentials.
// Keep the hexadecimal bytes in TTN's displayed order.
uint64_t joinEUI = 0x0000000000000000;
uint64_t devEUI  = 0x70B3D57ED0079771;

uint8_t appKey[] = {
  0xD9, 0x3B, 0xF9, 0x2C,
  0x8F, 0x7D, 0xDD, 0xA8,
  0x2E, 0x5D, 0x65, 0x06,
  0x36, 0xB5, 0x6B, 0x58
};

uint8_t nwkKey[] = {
  0xD0, 0xF3, 0xFA, 0x33,
  0x05, 0x2B, 0x81, 0x81,
  0xCC, 0xEB, 0xF9, 0x6E,
  0xFA, 0xCD, 0x4B, 0x75
};

static_assert(sizeof(appKey) == 16,
              "AppKey must contain exactly 16 bytes");

static_assert(sizeof(nwkKey) == 16,
              "NwkKey must contain exactly 16 bytes");

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