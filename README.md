# Zombie Slave – STM32 Automotive Control Firmware

Firmware for STM32-based control of Tesla 3-way water valves, coolant pumps, and other automotive peripherals.  
Originally developed for integration with the Zombie VCU, this project supports analog and digital IO, CAN communication, and ESP8266-based terminal access.

---

## 👷 Hardware Overview

- Tesla 3-way coolant valve
- Coolant pump with RPM control (manual or auto mode)
- Vacuum pump with hysteresis and warning detection
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

- **GP out 3** via **1k pull-up** is connected to input
- Output is 0–12V — **do not connect directly** to analog input
- Use a **3.9k resistor in series** for voltage protection
- Analog inputs are all **5V tolerant**

---

## 📄 License

This project is licensed under the **GNU General Public License v3**.  
See [`LICENSE`](LICENSE) for full terms.

---

## 🙌 Credits

Based on the [stm32-template](https://github.com/jsphuebner/stm32-template) by **Johannes Huebner**.  
Extended for Zombie VCU use cases and embedded control.

---

