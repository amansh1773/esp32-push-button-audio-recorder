# ESP32 Push-Button Audio Recorder

A push-button audio recorder built on an ESP32 Dev Module that captures audio from an I2S microphone, writes `.WAV` files to a MicroSD card over SPI, and includes an interactive HTML wiring and logic chart.

## Repository Structure

```text
esp32-push-button-audio-recorder/
├── README.md
├── firmware/
│   └── ESP32/
│       └── ESP32.ino
└── wiring/
    └── esp32_audio_recorder_diagram.html
```

## Hardware Required

* **ESP32 Dev Module** (30-pin silkscreen layout with `3V3`, `GND`, and `D`-prefixed pin labels)
* **I2S MEMS Microphone Module** (such as INMP441)
* **MicroSD Card Module** (SPI interface + FAT32-formatted MicroSD card)
* **Push Button** (momentary tactile switch)
* **Jumper wires and breadboard**

## Pinout & Wiring

The `3V3` rail and `GND` pins are shared across the modules as needed.

### Push Button

| Button Pin | ESP32 Silkscreen / GPIO | Notes |
| :--- | :--- | :--- |
| Pin 1 | `D4` (GPIO4) | Configured with `INPUT_PULLUP` |
| Pin 2 | `GND` | Active-LOW press to ground |

### MicroSD Card Module (SPI)

| MicroSD Pin | ESP32 Silkscreen / GPIO | Function |
| :--- | :--- | :--- |
| `VCC` | `3V3` | Power supply |
| `GND` | `GND` | Ground |
| `CS` | `D5` (GPIO5) | Chip Select |
| `MOSI` | `D23` (GPIO23) | SPI data out |
| `MISO` | `D19` (GPIO19) | SPI data in |
| `SCK` | `D18` (GPIO18) | SPI clock |

### I2S Microphone Module (INMP441)

| Mic Pin | ESP32 Silkscreen / GPIO | Function |
| :--- | :--- | :--- |
| `VDD` | `3V3` | Shared 3.3V rail |
| `GND` | `GND` | Ground |
| `L/R` | `GND` | Selects left channel |
| `WS` (LRC) | `D25` (GPIO25) | Word Select / Left-Right Clock |
| `SCK` (BCLK) | `D26` (GPIO26) | Bit Clock |
| `SD` (DOUT) | `D33` (GPIO33) | Serial Audio Data |

## How to Use

### 1. View the Wiring & Logic Chart
1. Open `wiring/esp32_audio_recorder_diagram.html` in any web browser (Chrome, Edge, Firefox).
2. Inspect the pin connections, shared `3V3`/`GND` bus, and recording state flow.

### 2. Flash the Arduino Sketch
1. Open `firmware/ESP32/ESP32.ino` in the **Arduino IDE** (with ESP32 board support installed).
2. Select your ESP32 board under **Tools -> Board** and choose the matching COM port under **Tools -> Port**.
3. Click **Upload**.
4. Open the **Serial Monitor** at `115200` baud to check SD card and I2S initialization messages.
5. **Press the button once** to start recording (`/rec_0001.wav`, `/rec_0002.wav`, etc.) and **press it again** to stop and finalize the `.WAV` file.
