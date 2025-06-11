# Zombie Slave – STM32 Automotive Control Firmware

Firmware for STM32-based control of Tesla 3-way water valves, coolant pumps, and other automotive peripherals.  
Originally developed for integration with the Zombie VCU, this project supports analog and digital IO, CAN communication, and ESP8266-based terminal access.

---

## 👷 Hardware Overview

- Tesla 3-way coolant valve
- Coolant pump with RPM control (manual or auto mode)
- Vacuum pump with hysteresis and warning detection (active only in Ready state)
- Voltage divider for analog signal monitoring
- Digital IO pins for output control and sensor reading

---

## 📦 Project Structure

```
include/               Header files
src/                   Source files
libopeninv/            External library for control logic
libopencm3/            Submodule for low-level STM32 hardware access
Makefile               Build system
linker.ld              STM32 linker script
.gitpod.yml            Gitpod workspace configuration
project.cbp            Code::Blocks project file
```

---

## ⚙️ Compiling

### 🔧 Toolchain

You will need the `arm-none-eabi` toolchain.

#### On Ubuntu:

```bash
sudo apt-get install git gcc-arm-none-eabi
```

---

### 📥 Download Dependencies

The only external dependencies are:

- [libopencm3](https://github.com/jsphuebner/libopencm3)
- [libopeninv](https://github.com/jsphuebner/libopeninv.git)

Fetch and build them with:

```bash
make get-deps
```

---

### 💠 Build Firmware

Build the `stm32_yourname` firmware binary:

```bash
make
```

Output files:
- `stm32_yourname.elf`
- `stm32_yourname.hex`
- `stm32_yourname.bin`

> These can be uploaded to your board via JTAG, updater.py script, or ESP8266 web interface.

---

## ⚡ Flashing

Flash the firmware using OpenOCD:

```bash
make flash
```

Ensure your OpenOCD configuration uses:
- `parport.cfg` as flasher interface
- `olimex_stm32_h103.cfg` for the board

---

## 📧 Terminal Access

The firmware includes a terminal accessible over USART3 or via ESP8266.

### Built-in Commands

- `set`, `get`, `flag`, `stream` — Parameter interaction
- `json` — JSON format export
- `save`, `load`, `defaults` — Flash parameter handling
- `reset`, `serial`, `errors`, `help` — System and debug tools

---

## 🧰 Hardware Notes

| Pin | Function                      | HW/SW Interface | Description                                                                                           | Code Reference            |
|-----|-------------------------------|------------------|-------------------------------------------------------------------------------------------------------|---------------------------|
| 01  | RS232 Rx                      |                  | RS232 receive line                                                                                   |                           |
| 02  | RS232 Tx                      |                  | RS232 transmit line                                                                                  |                           |
| 03  | GP Out 3                      | PD13             | Low-side switch (NCV8402ASTT1G) — Sinks to GND via N-MOSFET, 1k pull-up to 12V                       | `DigIo::tesla_coolant_valve_1_out` |
| 04  | GP Out 2                      | PD14             | Same as above                                                                                         | `DigIo::tesla_coolant_valve_2_out` |
| 05  | PWM 3                         | PB0              | High-side PWM (FAN3122TMX), 12V-level compatible                                                      | `DigIo::servo_pump_out`  |
| 06  | PWM 2                         | PA7              | High-side PWM (FAN3122TMX), 12V-level compatible                                                      |                           |
| 07  | PWM 1                         | PA6              | High-side PWM (FAN3122TMX), 12V-level compatible                                                      |                           |
| 08  | Analog 2 in                   | PC3              | 5V tolerant analog input, clamped to 3.3V, protected by resistor & diode                              |                           |
| 09  | Analog 1 in                   | PC2              | Same as above                                                                                         |                           |
| 10  | DAC 2                         | PA5              | Buffered via op-amp (TDA2320A), filtered output                                                       |                           |
| 11  | DAC 1                         | PA4              | Same as above                                                                                         |                           |
| 12  | MG2 Temp -                    |                  | Temperature sensor input                                                                              |                           |
| 13  | MG2 Temp +                    |                  | Temperature sensor input                                                                              |                           |
| 14  | MG1 Temp -                    |                  | Temperature sensor input                                                                              |                           |
| 15  | Ignition T15 In              | PD6              | 12V digital input, protected with voltage divider + cap                                               | `DigIo::ignition_in`      |
| 16  | REQ-                          |                  |                                                                                                       |                           |
| 17  | REQ+                          |                  |                                                                                                       |                           |
| 18  | CLK-                          |                  |                                                                                                       |                           |
| 19  | CLK+                          |                  |                                                                                                       |                           |
| 20  | MTH-                          |                  |                                                                                                       |                           |
| 21  | MTH+                          |                  |                                                                                                       |                           |
| 22  | HTM-                          |                  |                                                                                                       |                           |
| 23  | HTM+                          |                  |                                                                                                       |                           |
| 24  | LIN                           |                  |                                                                                                       |                           |
| 25  | CAN EXT 3 L                  |                  |                                                                                                       |                           |
| 26  | CAN EXT 3 H                  |                  |                                                                                                       |                           |
| 27  | CAN EXT L                    |                  |                                                                                                       |                           |
| 28  | CAN EXT H                    |                  |                                                                                                       |                           |
| 29  | MG1 Temp +                    |                  | Temperature sensor input                                                                              |                           |
| 30  | Oil Pump PWM                 | PE9              | 12V PWM via MMBT3904 NPN transistor                                                                  |                           |
| 31  | Neg Contactor LS Switch      | PD15             | Low-side switch (NCV8402ASTT1G)                                                                       | `DigIo::tesla_coolant_pump_out` |
| 32  | Inverter Power LS Switch     | PA8              | Low-side switch (NCV8402ASTT1G)                                                                       | `DigIo::ready_out`        |
| 33  | Main Contactor LS Switch     | PC7              | Low-side switch (NCV8402ASTT1G)                                                                       | `DigIo::condition_out`    |
| 34  | Precharge LS Switch          | PC6              | Low-side switch (NCV8402ASTT1G)                                                                       | `DigIo::cabin_heater_out` |
| 35  | Pot 2                         |                  | Digital potentiometer (AD5227BRJZ50-R2), SPI controlled, 0.1μF filtered                               |                           |
| 36  | Pot 1                         |                  | Same as above                                                                                         |                           |
| 37  | Trans SP                     | PD12             | Protected HS switch (NCV8461DR2G) with diagnostics                                                    |                           |
| 38  | Trans SL2-                   | PC8              | Low-side switch (NCV8402ASTT1G)                                                                       |                           |
| 39  | Trans SL1-                   | PC9              | Same as above                                                                                         |                           |
| 40  | Trans PB3                    |                  |                                                                                                       |                           |
| 41  | Trans PB2                    |                  |                                                                                                       |                           |
| 42  | Trans PB1                    |                  |                                                                                                       |                           |
| 43  | CAN EXT 2 L                 |                  |                                                                                                       |                           |
| 44  | CAN EXT 2 H                 |                  |                                                                                                       |                           |
| 45  | Throttle Ground              |                  |                                                                                                       |                           |
| 46  | Throttle 2                   | PC1              | 5V tolerant analog input, clamped & protected - 3.9k series resistor only 5V!                                                        | `AnaIn::tesla_coolant_valve_2_in` |
| 47  | Throttle 1                   | PC0              | 5V tolerant analog input, clamped & protected - 3.9k series resistor only 5V!                                                                  | `AnaIn::tesla_coolant_valve_1_in` |
| 48  | +5V Throttle                 |                  |                                                                                                       |                           |
| 49  | Brake Input                  | PA15             | 12V digital input, protected                                                                          | `DigIo::ready_safety_in`  |
| 50  | GP12V Input                  | PD4              | 12V digital input, protected                                                                          | `DigIo::vacuum_sensor_in`   |
| 51  | Hvrequest                    | PD5              | 12V digital input, protected                                                                          | `DigIo::cabin_heater_in`  |
| 52  | Start                        | PD7              | 12V digital input, protected                                                                          |                           |
| 53  | Reverse Direction            | PB3              | 12V digital input, protected                                                                          |                           |
| 54  | Forward Direction            | PB4              | 12V digital input, protected                                                                          |                           |
| 55  | Ground                       |                  |                                                                                                       |                           |
| 56  | Permanent +12V              |                  |                                                                                                       |                           |
| Int | 12V Supply Measurement       | PB1              | Voltage divider (8.2k/1.8k), filtered to scale to ADC                                                 | `AnaIn::dc_power_supply`    |
| Int | Status LED                   | PE2              | Push-pull output, 680Ω to GND                                                                         | `DigIo::led_out`          |



---

## 📄 License

This project is licensed under the **GNU General Public License v3**.  
See [`LICENSE`](LICENSE) for full terms.

---

## 🙌 Credits

Based on the [stm32-template](https://github.com/jsphuebner/stm32-template) by **Johannes Huebner**.  
Extended for Zombie VCU use cases and embedded control.

---

