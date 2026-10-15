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

.PHONY: help setup build upload monitor clean

help:
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

$(SKETCH)/secrets.h:
	@echo "Missing $(SKETCH)/secrets.h: copy $(SKETCH)/secrets.example.h and enter your TTN credentials." >&2
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
