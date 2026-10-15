# XIAO ESP32S3 LoRaWAN Temperature Sensor

This project is a low-cost, hands-on example of setting up the [XIAO ESP32S3 + Wio-SX1262 LoRa kit](https://www.seeedstudio.com/XIAO-ESP32S3-for-Meshtastic-LoRa-with-3D-Printed-Enclosure-p-6314.html) as a LoRaWAN end device. The example firmware joins TTN using OTAA and sends the ESP32's internal chip temperature—not ambient temperature. The ESP32 also makes the kit easy to extend with other sensors or actuators.

![Seeed Studio XIAO ESP32S3 and Wio-SX1262 LoRa kit](docs/img/SeeedXIAOESP32-S3Wio-SX1262LoRa-KitMeshtasticmitGehuse868-930MHz240MHz8MBWi-FiBLE5-380990_789x905.webp)

## Requirements

### Hardware

- Seeed Studio XIAO ESP32S3 for Meshtastic LoRa kit, with its Wio-SX1262 board
- The kit's LoRa antenna, connected to the SX1262 before powering the radio
- USB-C **data** cable and a computer for flashing
- A single-cell LiPo battery only if battery operation is needed; USB power is sufficient for setup
- A TTN-compatible LoRaWAN gateway reachable in the device's region (EU868 is configured in this project)

The kit uses a board-to-board (B2B) connector. Seat the XIAO and Wio-SX1262 together as designed; do not substitute the Wio-SX1262 board intended for the XIAO nRF52840. Attach the LoRa antenna before transmitting. See [Seeed's kit guide](https://wiki.seeedstudio.com/wio_sx1262_with_xiao_esp32s3_kit/) for the board and antenna details.

### Software and accounts

- [Arduino CLI](https://arduino.github.io/arduino-cli/) and the [TTN CLI](https://www.thethingsindustries.com/docs/the-things-stack/interact/cli/) (`ttn-lw-cli`)
- `make`, `python3`, `jq` and `openssl`
- A [The Things Network account](https://www.thethingsnetwork.org/)

On macOS, install them with `brew install arduino-cli jq` and `brew tap TheThingsNetwork/lorawan-stack && brew trust thethingsnetwork/lorawan-stack && brew install ttn-lw-stack`. You do not need to install the ESP32 toolchain or RadioLib manually; `make setup` installs them.

## Install and register the device

### 1. Install the build dependencies

From the project folder, run `make setup`. This installs the ESP32 Arduino core (3.3.8) and RadioLib 6.6.0, pinned in the `Makefile`.

### 2. Assemble and connect the kit

Fit the Wio-SX1262 to the XIAO ESP32S3 using its B2B connector. Connect the kit's LoRa antenna, then connect the XIAO to your computer using a USB-C data cable. Keep the antenna attached whenever the radio is powered and transmitting.

### 3. Register the device in TTN

1. Run `make ttn-login`. This opens your browser to authorize the TTN CLI on the `eu1` cluster. For another cluster, run `make ttn-login TTN_HOST=nam1.cloud.thethings.network`.
2. Run `make ttn-provision`. It creates the TTN application `xiao-temperature` and the OTAA device `xiao-temperature-01` (LoRaWAN 1.1.0, EU868), generates random EUIs and keys, installs the uplink payload formatter, and writes the credentials to `firmware/secrets.h`. Override names with `make ttn-provision APP_ID=my-app DEVICE_ID=my-device-01`.

The script stops if `firmware/secrets.h` already exists, because that file belongs to a registered device. Pass `FORCE=1` to back it up (as `secrets.h.bak.<timestamp>`) and replace it with a new device.

You need a gateway connected to the same TTN cluster and using the frequency plan for your region. The firmware as provided is configured for **EU868**: it starts the radio at 868.1 MHz and uses the EU868 LoRaWAN region. Do not use these settings in another region without changing the firmware and matching its TTN frequency plan to local regulations.

**Protect the keys.** Do not commit `firmware/secrets.h` or share a firmware image built with your device's credentials. This repository ignores that local file. Do not erase the board's saved LoRaWAN nonce state while it keeps using the same device; nonce persistence is required for safe OTAA operation.

<details>
<summary>Manual setup in the TTN Console instead</summary>

1. In the [TTN Console](https://console.cloud.thethings.network/), create an application (for example `xiao-temperature`).
2. Register an end device manually: frequency plan **Europe 863-870 MHz (SF9 for RX2 - recommended)**, LoRaWAN **1.1.0**, regional parameters **RP001 1.1.0-B**, and generated JoinEUI, DevEUI, AppKey and NwkKey.
3. Copy `firmware/secrets.example.h` to `firmware/secrets.h` and enter the same JoinEUI and DevEUI and the 32-hex-digit keys, with no spaces or `0x` prefixes.
4. In the application's **Payload formatters**, choose **Custom Javascript** for the uplink formatter and paste [`firmware/ttn_payload_formatter.js`](firmware/ttn_payload_formatter.js).

</details>

### 4. Build and flash the firmware

1. Run `make build` to compile the firmware.
2. Connect the XIAO by USB-C and run `make upload`. The serial port is detected automatically; if not, pass it explicitly, for example `make upload PORT=/dev/cu.usbmodem1101`.
3. Run `make monitor` to view startup and join messages (115200 baud).

If the board does not appear as a serial port or upload does not start, try another USB data cable and port. Because the firmware sleeps between uplinks (which turns USB off), a running device can reject an automatic upload: put the XIAO into its USB bootloader mode (hold BOOT, tap RESET, release BOOT), then retry.

### 5. Confirm the OTAA join and uplinks

After startup, the serial monitor should report radio initialization and attempt to join TTN. A successful join is reported as **Joined TTN!**. Then the device sends an unconfirmed, two-byte temperature uplink on **FPort 1**, with a nominal interval of 10 minutes; regional airtime limits can make the interval longer. It uses the ESP32's on-chip temperature reading, which is not a calibrated ambient-temperature sensor.

In TTN, open the end device's **Live data** tab (or the Console under **xiao-temperature**). Look for a join event followed by uplink events; decoded uplinks contain `temperature_c`. TTN gateway coverage is required for both the join and uplinks. An uplink can be received without a downlink; “no downlink received” is normal for this unconfirmed sensor.

The payload is a signed 16-bit integer in big-endian order, representing hundredths of a degree Celsius, decoded by the formatter that `make ttn-provision` installs. For example, a payload of `09 C4` decodes to `25.00 °C`.

## Troubleshooting

- **Radio initialization fails:** Check that the Wio-SX1262 B2B connector is fully seated, the kit's radio antenna is attached, and the configured board is the XIAO ESP32S3/Wio-SX1262 combination.
- **OTAA join does not succeed:** Verify TTN gateway coverage and matching frequency plans, JoinEUI, DevEUI, AppKey, NwkKey, LoRaWAN version, and region. Use TTN Live data to inspect join-request and join-accept events. Avoid rapid repeated restarts; the firmware bounds retries and backs off between attempts.
- **The monitor shows nonce or stored-state errors:** Do not erase flash or disable TTN nonce checks. The firmware stores join nonce history in ESP32 nonvolatile storage, together with the DevEUI it belongs to, and relies on it to avoid invalid OTAA joins.
- **`ERR_JOIN_NONCE_INVALID` or TTN `dev_nonce_too_small` after re-registering a device:** TTN rejects join requests whose nonce is not higher than the last one it saw, so a device that was deleted and re-created with the same keys or EUIs cannot reuse old counters. Always register a new device (a fresh DevEUI via `make ttn-provision FORCE=1`) instead of reusing an old one. The firmware discards stored nonces automatically when the DevEUI in `firmware/secrets.h` changes, as the log message `Saved nonces belong to another device identity: discarding.` shows.
- **Upload fails with “No serial data received”:** the firmware sleeps between uplinks, which turns USB off. Hold BOOT, tap RESET, release BOOT, and run `make upload` again.
- **No temperature appears in decoded data:** Confirm an uplink is present, the uplink formatter is saved, and the payload has exactly two bytes on FPort 1.

## Project files

- `Makefile` — setup, TTN login/provisioning, build, upload, and monitor commands
- `scripts/ttn-provision.sh` — creates the TTN application and device with `ttn-lw-cli` and writes `firmware/secrets.h`
- `firmware/firmware.ino` — LoRaWAN OTAA, persistent join nonces, sleep, and temperature uplinks
- `firmware/config.h` — radio pin map, region, timing, and credentials include
- `firmware/secrets.example.h` — template for the ignored local `firmware/secrets.h` credentials file
- `firmware/ttn_payload_formatter.js` — TTN uplink decoder
