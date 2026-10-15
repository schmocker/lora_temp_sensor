#include <Arduino.h>
#include <SPI.h>
#include <Preferences.h>
#include <math.h>
#include <esp_sleep.h>
#include "config.h"

Preferences store;

void haltDevice(const char* message) {
  Serial.println(message);
  Serial.println("Stopped to avoid unsafe or repeated transmissions.");

  while (true) {
    delay(1000);
  }
}

void printResult(const char* message, int16_t state) {
  Serial.printf("%s: %s (%d)\n",
                message,
                stateDecode(state).c_str(),
                state);
}

void lowPowerWait(uint32_t waitMs) {
  if (waitMs == 0) {
    return;
  }

  Serial.flush();

  int16_t state = radio.sleep(true);
  if (state != RADIOLIB_ERR_NONE) {
    printResult("Radio sleep failed", state);
    haltDevice("Cannot safely enter low-power wait.");
  }

  esp_err_t stateResult = esp_sleep_enable_timer_wakeup(
    (uint64_t)waitMs * 1000ULL
  );
  if (stateResult != ESP_OK) {
    Serial.printf("Sleep timer setup failed: %d\n", stateResult);
    haltDevice("Cannot set the low-power wake timer.");
  }

  stateResult = esp_light_sleep_start();
  if (stateResult != ESP_OK) {
    Serial.printf("Light sleep failed: %d\n", stateResult);
    haltDevice("Cannot enter low-power mode.");
  }

  state = radio.standby();
  if (state != RADIOLIB_ERR_NONE) {
    printResult("Radio wake failed", state);
    haltDevice("Cannot wake the LoRa radio from sleep.");
  }
}

void saveNonces() {
  uint8_t* buffer = node.getBufferNonces();

  size_t written = store.putBytes(
    "nonces",
    buffer,
    RADIOLIB_LORAWAN_NONCES_BUF_SIZE
  );

  if (written != RADIOLIB_LORAWAN_NONCES_BUF_SIZE) {
    haltDevice("ERROR: Could not save LoRaWAN nonces to flash.");
  }

  Serial.println("Join nonces saved to flash.");
}

void restoreNonces() {
  if (!store.isKey("nonces")) {
    Serial.println("No saved nonces: first run of this firmware.");
    return;
  }

  size_t length = store.getBytesLength("nonces");

  if (length != RADIOLIB_LORAWAN_NONCES_BUF_SIZE) {
    haltDevice(
      "ERROR: Saved nonce buffer has an unexpected size. "
      "Do not erase it and retry with the same device identity."
    );
  }

  uint8_t buffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];

  size_t loaded = store.getBytes(
    "nonces",
    buffer,
    sizeof(buffer)
  );

  if (loaded != sizeof(buffer)) {
    haltDevice("ERROR: Could not read saved LoRaWAN nonces.");
  }

  int16_t state = node.setBufferNonces(buffer);

  if (state != RADIOLIB_ERR_NONE) {
    printResult("Restoring nonces failed", state);

    haltDevice(
      "Saved nonces are invalid or do not match this configuration. "
      "Check the device credentials and RadioLib version. "
      "Do not disable TTN nonce checks."
    );
  }

  Serial.println("Saved join nonces restored.");
}

void joinNetwork() {
  if (!decodeLoRaWANKey(appKeyHex, appKey) ||
      !decodeLoRaWANKey(nwkKeyHex, nwkKey)) {
    haltDevice(
      "ERROR: AppKey and NwkKey must each be exactly 32 hexadecimal characters."
    );
  }

  // Call once, before restoring the stored nonce buffer.
  node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);

  restoreNonces();

  // Bound retries so an unavailable gateway cannot cause
  // endless join traffic.
  const uint8_t maxAttempts = 6;

  for (uint8_t attempt = 0; attempt < maxAttempts; attempt++) {
    Serial.printf(
      "\nJoining TTN using OTAA, attempt %u/%u...\n",
      (unsigned int)(attempt + 1),
      (unsigned int)maxAttempts
    );

    int16_t state = node.activateOTAA();

    // Save after every attempt, including when the join request
    // was transmitted but no join-accept was received.
    saveNonces();

    printResult("Join result", state);

    if (state == RADIOLIB_LORAWAN_NEW_SESSION ||
        state == RADIOLIB_LORAWAN_SESSION_RESTORED ||
        (state == RADIOLIB_ERR_NONE && node.isActivated())) {
      Serial.println("Joined TTN!");
      Serial.printf("DevAddr: %08lX\n",
                    (unsigned long)node.getDevAddr());
      return;
    }

    if (attempt + 1 < maxAttempts) {
      // Backoff: 5, 10, 20, 40, then 60 minutes.
      uint32_t retrySeconds = 300UL << attempt;

      if (retrySeconds > 3600UL) {
        retrySeconds = 3600UL;
      }

      Serial.printf(
        "No successful join. Retrying in %lu minutes.\n",
        (unsigned long)(retrySeconds / 60UL)
      );

      Serial.println(
        "Check TTN Live data for the actual rejection or downlink error."
      );

      delay(retrySeconds * 1000UL);
    }
  }

  haltDevice(
    "Join attempt limit reached. Check TTN and gateway logs "
    "before restarting the board."
  );
}

void setup() {
  Serial.begin(115200);

  // Allow USB serial to connect, but do not require a computer
  // to be attached for the sensor to run.
  unsigned long started = millis();

  while (!Serial && millis() - started < 10000UL) {
    delay(10);
  }

  Serial.println("\nXIAO chip-temperature LoRaWAN sensor");

  // Same hardware setup as your successful radio test.
  pinMode(LORA_RF_SW, OUTPUT);
  digitalWrite(LORA_RF_SW, HIGH);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI);

  Serial.println("Initializing SX1262...");

  int16_t state = radio.begin(868.1);

  if (state != RADIOLIB_ERR_NONE) {
    printResult("Radio initialization failed", state);
    haltDevice("Check the Wio module connection and radio pins.");
  }

  Serial.println("Radio initialization OK");

  // Open ESP32 nonvolatile storage in read/write mode.
  if (!store.begin(STORAGE_NAMESPACE, false)) {
    haltDevice("ERROR: Could not open ESP32 persistent storage.");
  }

  // Configure uplinks and join attempts before starting OTAA.
  node.setADR(true);
  node.setDutyCycle(true, 1250);
  node.setDeviceStatus(0);

  joinNetwork();

  Serial.println("Ready to send chip temperature.");
}

void loop() {
  uint32_t waitMs = node.timeUntilUplink();

  if (waitMs > 0) {
    Serial.printf("Airtime wait: %lu seconds\n",
                  (unsigned long)((waitMs + 999UL) / 1000UL));
    lowPowerWait(waitMs);
  }

  float temperature = temperatureRead();

  if (!isfinite(temperature) ||
      temperature < -327.68f ||
      temperature > 327.67f) {
    Serial.println("Invalid temperature: skipping this reading.");
    lowPowerWait(uplinkIntervalSeconds * 1000UL);
    return;
  }

  // Signed int16, hundredths of a degree C, big-endian.
  int16_t temperature100 =
    (int16_t)lroundf(temperature * 100.0f);

  uint16_t encoded = (uint16_t)temperature100;

  uint8_t payload[2] = {
    (uint8_t)(encoded >> 8),
    (uint8_t)(encoded & 0xFF)
  };

  Serial.printf("\nSending chip temperature: %.2f C\n",
                temperature);

  Serial.printf("Payload: %02X %02X, FPort: 1\n",
                payload[0], payload[1]);

  uint32_t frameCountBefore = node.getFCntUp();

  // Unconfirmed uplink, followed by both receive windows.
  int16_t state = node.sendReceive(
    payload,
    sizeof(payload),
    1,
    false
  );

  // Persist nonce state after the exchange as well.
  // Session frame counters are NOT restored after reboot:
  // the device performs a fresh OTAA join instead.
  saveNonces();

  if (state == RADIOLIB_LORAWAN_NO_DOWNLINK) {
    Serial.println(
      "Uplink transmitted; no downlink received (normal)."
    );
  } else if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Uplink exchange completed with a downlink.");
  } else {
    printResult("Uplink exchange returned an error", state);
  }

  if (node.getFCntUp() != frameCountBefore) {
    Serial.println(
      "Radio transmitted a frame. Check TTN to confirm delivery."
    );
  }

  Serial.printf("Next attempt in %lu seconds, subject to airtime limits.\n",
                (unsigned long)uplinkIntervalSeconds);
  lowPowerWait(uplinkIntervalSeconds * 1000UL);
}