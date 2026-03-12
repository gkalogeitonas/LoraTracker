Here is a structured, step-by-step implementation plan. You can copy and paste this directly into GitHub Copilot (or Copilot Chat) to give it the exact context, pin mappings, and logic flow it needs to generate a clean, working Arduino sketch.

### Prompt/Plan for GitHub Copilot

---

**Context:**
Write an Arduino sketch for an ESP32 (specifically the TTGO LoRa32 V2.1.6 board) connected to an LC76G GPS module. The code needs to read GPS data via a secondary UART, display the coordinates on the onboard OLED, and transmit the location via LoRa at configurable intervals.

**1. Hardware & Pin Definitions**

* **GPS UART:** RX pin is `34`, TX pin is `4`. Default baud rate is `115200`. Use `HardwareSerial(1)`.
* **GPS Control Pins:** RST pin is `25`, PPS pin is `36` (set up PPS as an input, but we will just monitor it or leave it idle for now).
* **LoRa (SX1276):** SCK=`5`, MISO=`19`, MOSI=`27`, CS=`18`, RST=`23`, DIO0=`26`.
* **OLED (SSD1306):** SDA=`21`, SCL=`22`. Screen size is 128x64.

**2. Configuration Parameters (Make these global constants)**

* `DISPLAY_INTERVAL_MS`: Set to 2000 (Update screen every 2 seconds).
* `TX_INTERVAL_MS`: Set to 10000 (Transmit LoRa packet every 10 seconds).
* `LORA_FREQ`: `868E6` (868 MHz).
* `LORA_SF`: `7` (Spreading Factor).
* `LORA_SYNC_WORD`: `0xF3` (Custom sync word to isolate our network).

**3. Required Libraries**

* `TinyGPSPlus` for parsing NMEA data.
* `LoRa` (sandeepmistry) for LoRa communication.
* `Adafruit_SSD1306` and `Adafruit_GFX` for the OLED display.
* `Wire` and `SPI`.

**4. `setup()` Function Logic**

* Initialize standard `Serial` at `115200` for debugging.
* Set GPS RST pin (`25`) to `OUTPUT`, pull LOW for 100ms, then HIGH to reset the module. Delay 500ms.
* Initialize `HardwareSerial(1)` with the GPS pins and baud rate.
* Initialize the I2C OLED display. Show a "Booting..." message.
* Override default SPI pins for the LoRa module (`SPI.begin(SCK, MISO, MOSI, CS)`).
* Initialize LoRa with `LoRa.setPins(CS, RST, DIO0)`. Start LoRa at `LORA_FREQ`.
* Apply LoRa settings: `LoRa.setSpreadingFactor(LORA_SF)` and `LoRa.setSyncWord(LORA_SYNC_WORD)`.

**5. `loop()` Function Logic (Use non-blocking `millis()` timers)**

* **Continuous task:** Read from the GPS hardware serial and feed it to the `TinyGPSPlus` object `encode()` method.
* **Display Task:** If `DISPLAY_INTERVAL_MS` has passed, clear the OLED and print the current Latitude, Longitude, Altitude, and number of visible satellites. If GPS data is invalid, display "Waiting for Fix...". Update the timer variable.
* **Transmit Task:** If `TX_INTERVAL_MS` has passed, construct a string with the Lat/Lon data (e.g., "LAT:xx.xx,LON:yy.yy"). Begin a LoRa packet, print the string to the LoRa radio, and end the packet. Print a debug message to the Serial monitor. Update the timer variable.

---

### Why this plan works well for Copilot:

1. **Hardcoded Hardware:** AI often guesses the wrong default pins for the TTGO board (especially the SPI pins for the LoRa chip). Laying them out explicitly prevents compiling errors.
2. **Global Constants:** Grouping the configuration parameters at the top ensures Copilot creates variables you can easily tweak later without digging through the main code.
3. **Non-blocking logic:** Explicitly asking for `millis()` timers prevents Copilot from using `delay()`, which would freeze the `loop()` and cause the ESP32 to drop incoming GPS serial data, resulting in missing or broken coordinates.

Would you like me to suggest a specific data format for your LoRa transmission payload (like a compact JSON or comma-separated string) to make it easier for the receiving board to parse?