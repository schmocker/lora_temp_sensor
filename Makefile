SKETCH   := firmware
FQBN     := esp32:esp32:XIAO_ESP32S3
CORE     := esp32:esp32@3.3.8
RADIOLIB := RadioLib@6.6.0
BAUD     := 115200
BUILD    := build

# Serial port autodetection; override with: make upload PORT=/dev/ttyACM0
PORT ?= $(shell arduino-cli board list --format json 2>/dev/null | \
	python3 -c "import json,sys; d=json.load(sys.stdin); \
	p=[x['port']['address'] for x in (d.get('detected_ports') or []) if 'esp32' in str(x).lower()]; \
	print(p[0] if p else '')")

TTN_HOST ?= eu1.cloud.thethings.network
export TTN_HOST

.PHONY: help setup build upload monitor clean ttn-login ttn-provision

help:
	@echo "make ttn-login      log in to TTN with ttn-lw-cli (opens a browser)"
	@echo "make ttn-provision  create TTN app + device, write $(SKETCH)/secrets.h"
	@echo "make setup    install ESP32 core and RadioLib"
	@echo "make build    compile the firmware"
	@echo "make upload   build and flash (PORT=... to override)"
	@echo "make monitor  open serial monitor"
	@echo "make clean    remove build output"

setup:
	arduino-cli core update-index \
		--additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
	arduino-cli core install $(CORE) \
		--additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
	arduino-cli lib install $(RADIOLIB)

ttn-login:
	ttn-lw-cli use $(TTN_HOST) --user --overwrite
	ttn-lw-cli login

# Override with APP_ID=..., DEVICE_ID=..., FORCE=1 (replace existing secrets.h)
ttn-provision:
	./scripts/ttn-provision.sh

$(SKETCH)/secrets.h:
	@echo "Missing $(SKETCH)/secrets.h: run 'make ttn-provision' (or copy $(SKETCH)/secrets.example.h and enter your credentials)." >&2
	@exit 1

build: $(SKETCH)/secrets.h
	arduino-cli compile --fqbn $(FQBN) --build-path $(BUILD) $(SKETCH)

upload: build
	@test -n "$(PORT)" || { echo "No board found; connect it or pass PORT=..." >&2; exit 1; }
	arduino-cli upload --fqbn $(FQBN) --port $(PORT) --input-dir $(BUILD) $(SKETCH)

monitor:
	@test -n "$(PORT)" || { echo "No board found; connect it or pass PORT=..." >&2; exit 1; }
	arduino-cli monitor --port $(PORT) --config baudrate=$(BAUD)

clean:
	rm -rf $(BUILD)
