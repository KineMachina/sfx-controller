# Bill of Materials (BOM) - Kinemachina SFX Controller

**Project:** Kinemachina ESP32-S3 SFX Controller  
**Revision:** 1.0  
**Date:** 2024  
**PCB Assembly:** Required

---

## Active Components

| Ref Designator | Qty | Part Number | Description | Package | Manufacturer | Notes |
|----------------|-----|-------------|-------------|---------|--------------|-------|
| U1 | 1 | ESP32-S3-WROOM-1-N16R8 | ESP32-S3 MCU, 16MB Flash, 8MB PSRAM | QFN-56 | Espressif | Or ESP32-S3-DevKitC-1 module |
| U2 | 1 | PCM5122 | I2S Audio DAC, 32-bit, 384kHz | QFN-32 | Texas Instruments | Alternative: PCM5102A (simpler, no I2C) |
| U3 | 1 | AMS1117-3.3 | 3.3V LDO Regulator, 1A | SOT-223 | AMS | For 3.3V power |
| U4 | 1 | AMS1117-5.0 | 5V LDO Regulator, 1A | SOT-223 | AMS | For 5V power (NeoPixels) |
| U5 | 1 | SD Card Socket | MicroSD Card Socket | Through-hole | Various | Alternative: Surface mount |
| J1 | 1 | USB-C Connector | USB-C Receptacle | Through-hole | Various | For programming/power |
| J2 | 1 | 3.5mm Audio Jack | Stereo Audio Jack | Through-hole | Various | For audio output |
| J3 | 1 | JST XH-2.54 | 2-pin Connector | Through-hole | JST | NeoPixel power/data |
| J4 | 1 | JST XH-2.54 | 2-pin Connector | Through-hole | JST | External power input (optional) |
| SW1 | 1 | Tactile Switch | Reset Button, 6x6mm | SMD | Various | Reset switch |
| SW2 | 1 | Tactile Switch | Boot Button, 6x6mm | SMD | Various | Boot mode switch |

---

## Passive Components

| Ref Designator | Qty | Value | Description | Package | Notes |
|----------------|-----|-------|-------------|---------|-------|
| R1-R4 | 4 | 10kΩ | Pull-up Resistors | 0603 | I2C, SPI pull-ups |
| R5 | 1 | 470Ω | Current Limiting Resistor | 0603 | NeoPixel data line |
| R6-R8 | 3 | 22Ω | Series Resistors | 0603 | I2S signal integrity |
| R9 | 1 | 0Ω | Zero Ohm Jumper | 0603 | Optional, for routing |
| C1-C3 | 3 | 100nF | Decoupling Capacitors | 0603 | Power supply decoupling |
| C4-C5 | 2 | 10µF | Bulk Capacitors | 0805 | Power supply filtering |
| C6-C7 | 2 | 22pF | Crystal Load Capacitors | 0603 | ESP32 crystal (if external) |
| C8-C9 | 2 | 100µF | Electrolytic Capacitors | 6.3x5.4mm | Power supply smoothing |
| C10 | 1 | 1µF | Bypass Capacitor | 0603 | PCM5122 analog supply |
| C11-C12 | 2 | 220µF | Audio Coupling Capacitors | 6.3x5.4mm | Audio output coupling |
| L1 | 1 | 10µH | Inductor | 0805 | Power supply filtering (optional) |
| Y1 | 1 | 40MHz | Crystal Oscillator | HC-49/SMD | ESP32 external crystal (if needed) |

---

## Optional Components (Future Enhancements)

| Ref Designator | Qty | Part Number | Description | Package | Notes |
|----------------|-----|-------------|-------------|---------|-------|
| U6 | 1 | ST7789 | 2" Color IPS Display, 240x320 | Module | Optional - Display enhancement |
| J5 | 1 | FPC Connector | Display FPC Connector | SMD | For ST7789 display |
| R10 | 1 | 10kΩ | Potentiometer | Through-hole | Volume control (optional) |
| LED1 | 1 | LED, Red | Power Indicator | 0603 | Status LED |
| LED2 | 1 | LED, Blue | WiFi Status | 0603 | Connection indicator |

---

## Connectors & Headers

| Ref Designator | Qty | Description | Pitch | Notes |
|----------------|-----|-------------|-------|-------|
| H1 | 1 | 2x20 Pin Header | 2.54mm | ESP32-S3 DevKit (if using module) |
| H2 | 1 | 1x4 Pin Header | 2.54mm | I2S Debug (optional) |
| H3 | 1 | 1x6 Pin Header | 2.54mm | SPI Debug (optional) |
| H4 | 1 | 1x2 Pin Header | 2.54mm | UART (optional) |

---

## Mechanical Components

| Ref Designator | Qty | Description | Notes |
|----------------|-----|-------------|-------|
| - | 4 | M3 Standoffs, 10mm | PCB mounting |
| - | 4 | M3 Screws | PCB mounting |
| - | 4 | M3 Nuts | PCB mounting |
| - | 1 | Enclosure | Custom or off-the-shelf |

---

## External Components (Not on PCB)

| Item | Qty | Description | Notes |
|------|-----|-------------|-------|
| - | 1 | MicroSD Card | FAT32 formatted, 8GB+ recommended |
| - | 1 | NeoPixel Strip | WS2812B or compatible, configurable count |
| - | 1 | Audio Amplifier | Class D recommended (e.g., PAM8403, MAX98357A) |
| - | 1 | Speakers | 4-8Ω impedance |
| - | 1 | Power Supply | 5V, 3A+ (depends on NeoPixel count) |
| - | 1 | USB-C Cable | For programming/power |

---

## Pin Assignment Reference

### ESP32-S3 Pin Mapping

| Function | Pin | Notes |
|----------|-----|-------|
| I2S BCK | GPIO7 | Bit Clock |
| I2S LRC | GPIO15 | Left/Right Clock |
| I2S DOUT | GPIO16 | Data Out |
| SD CS | GPIO10 | Chip Select |
| SPI MOSI | GPIO11 | Master Out Slave In |
| SPI MISO | GPIO13 | Master In Slave Out |
| SPI SCK | GPIO12 | Serial Clock |
| I2C SDA | GPIO8 | PCM5122 SDA (anti-pop mute) |
| I2C SCL | GPIO9 | PCM5122 SCL (anti-pop mute) |
| NeoPixel Data | GPIO5 | WS2812B Data |
| UART TX | GPIO43 | Serial output (USB-CDC) |
| UART RX | GPIO44 | Serial input (USB-CDC) |

### PCM5122 Pin Mapping

| Function | Pin | ESP32 Connection |
|----------|-----|------------------|
| BCLK | Pin 5 | GPIO7 (I2S BCK) |
| LRCK | Pin 6 | GPIO15 (I2S LRC) |
| DIN | Pin 7 | GPIO16 (I2S DOUT) |
| SCL | Pin 16 | GPIO9 (I2C SCL) |
| SDA | Pin 17 | GPIO8 (I2C SDA) |
| VDD | Pin 1 | 3.3V |
| AGND | Pin 2 | Ground |
| VOUTL | Pin 20 | Left Audio Out |
| VOUTR | Pin 19 | Right Audio Out |

---

## Power Requirements

| Rail | Voltage | Current | Notes |
|------|---------|---------|-------|
| 3.3V | 3.3V | 500mA | ESP32-S3, PCM5122, SD card |
| 5V | 5.0V | 2-3A | NeoPixels (depends on count) |
| Input | 5V USB-C | 3A+ | Total system power |

**Power Calculation:**
- ESP32-S3: ~200mA @ 3.3V
- PCM5122: ~50mA @ 3.3V
- SD Card: ~100mA @ 3.3V
- NeoPixels: ~60mA per pixel @ 5V (8 pixels = 480mA)
- Total: ~500mA @ 3.3V, ~500mA @ 5V

---

## PCB Specifications

- **Layers:** 2-layer minimum (4-layer recommended for better power distribution)
- **Thickness:** 1.6mm standard
- **Copper Weight:** 1oz (35µm)
- **Surface Finish:** HASL or ENIG (ENIG recommended for better reliability)
- **Solder Mask:** Green (standard)
- **Silkscreen:** White
- **Minimum Trace Width:** 0.15mm (6 mil)
- **Minimum Via Size:** 0.2mm (8 mil)
- **Clearance:** 0.15mm (6 mil)

---

## Assembly Notes

1. **ESP32-S3 Module:** Can use pre-assembled DevKit module or bare chip
2. **PCM5122:** Requires careful layout for audio quality (keep analog traces short)
3. **Power Supply:** Ensure adequate decoupling near all ICs
4. **NeoPixel:** Add 470Ω resistor on data line, 1000µF capacitor near strip power
5. **SD Card:** Use proper pull-up resistors on SPI lines
6. **Audio Output:** Use shielded cable for audio connections
7. **Ground Plane:** Use solid ground plane for noise reduction

---

## Recommended PCB Manufacturers

- **JLCPCB** - Good for prototypes, low cost
- **PCBWay** - Good quality, reasonable pricing
- **OSH Park** - US-based, high quality
- **Seeed Studio** - Good for small batches

---

## Recommended Component Suppliers

- **Digi-Key** - Full component selection
- **Mouser** - Good for active components
- **LCSC** - Good for passives and connectors
- **AliExpress** - Budget components (verify authenticity)

---

## Design Files Needed

- **Schematic** (.sch or .pdf)
- **PCB Layout** (.brd, .kicad_pcb, or Gerber files)
- **BOM** (this file)
- **Assembly Drawing** (.pdf)
- **Pick and Place File** (.csv or .txt)
- **Gerber Files** (for PCB fabrication)

---

## Revision History

| Rev | Date | Changes | Author |
|-----|------|---------|--------|
| 1.0 | 2024 | Initial BOM | - |

---

## Notes

- All component values are nominal. Verify against actual schematic.
- Package sizes are recommendations; adjust based on PCB space and assembly method.
- Some components may be optional depending on final design requirements.
- For production, consider using 0402 packages for space savings.
- Add test points for debugging (power rails, signals).
- Consider adding ESD protection on USB and audio connectors.
