# Module Template

## Purpose
Use this when creating a new software module for this repository. A module normally consists of:
- `include/module_name.h`: class declaration, constants, small inline-only modules if simple.
- `src/module_name.cpp`: implementation for CAN parsing, control logic, and periodic work.
- Optional entries in `include/param_prj.h`, `include/errormessage_prj.h`, `src/main.cpp`, and `Makefile`.

## Existing Module Patterns
- `TeensyBMS`: derives from `BMS`, registers CAN RX frames, validates CRC, decodes into cached state, publishes many `Param` values every 100 ms, sends one status frame.
- `TeslaDCDC`: derives from `DCDC`, decodes a status frame directly into `Param`, posts errors, sends a command frame every 500 ms from `Task100Ms()`.
- `mVCUIntegration`: standalone class, validates CRC/counter, handles RX timeout, bridges charger/heater parameters to CAN frames.
- `VWMLBClass`: larger protocol emulator, keeps protocol structs internally, uses `Task10Ms()` and `Task100Ms()` counters to send messages at DBC-defined periods.
- `Heater`, `LVDU`, `TeslaCoolantPump`, `TeslaValve`: header-only control/state modules using `Param`, `DigIo`, and `AnaIn`.

## File Naming
- Header guard follows uppercase filename style: `MODULE_NAME_H`.
- Include path uses quotes for project headers: `#include "params.h"`.
- Class names are usually descriptive and hardware/protocol-oriented: `TeslaDCDC`, `TeensyBMS`, `mVCUIntegration`.
- Object name in `main.cpp` uses lower camel case: `static NewModule newModule;`.

## Header Skeleton
Use inheritance only when there is an existing project interface (`BMS`, `DCDC`, `chargerhw`, etc.). Otherwise use a standalone class.

```cpp
#ifndef NEW_MODULE_H
#define NEW_MODULE_H

#include <stdint.h>
#include "canhardware.h"
#include "params.h"

#define NEW_MODULE_RX_TIMEOUT_TICKS_100MS 5

class NewModule
{
public:
    void SetCanInterface(CanHardware* c);
    void DecodeCAN(int id, uint8_t* data, uint8_t dlc);
    void Task1Ms() {}
    void Task10Ms() {}
    void Task100Ms();

private:
    bool CheckCrc(const uint8_t* data) const;
    bool ValidateCounter(uint8_t counter);
    void PublishParams(bool dataValid);
    void SendStatus();

    CanHardware* can = nullptr;
    uint8_t txCounter = 0;
    uint8_t lastRxCounter = 0;
    uint8_t rxTimeoutTicks = 0;
    bool haveSeenValidCounter = false;

    float measuredVoltage = 0.0f;
    float measuredCurrent = 0.0f;
    uint8_t faultBits = 0;
};

#endif // NEW_MODULE_H
```

## Implementation Skeleton

```cpp
#include "new_module.h"
#include "errormessage.h"
#include <libopencm3/stm32/crc.h>
#include <string.h>

#define NEW_MODULE_STATUS_ID 0x123
#define NEW_MODULE_COMMAND_ID 0x456

void NewModule::SetCanInterface(CanHardware* c)
{
    if (c == nullptr)
    {
        can = nullptr;
        rxTimeoutTicks = 0;
        haveSeenValidCounter = false;
        Param::SetInt(Param::new_module_data_valid, 0);
        return;
    }

    can = c;
    can->RegisterUserMessage(NEW_MODULE_STATUS_ID);
}

bool NewModule::CheckCrc(const uint8_t* data) const
{
    uint32_t buf[2] = {0, 0};
    memcpy(buf, data, 8);
    buf[1] &= 0x00FFFFFF; // clear CRC byte at byte 7
    crc_reset();
    return (crc_calculate_block(buf, 2) & 0xFFU) == data[7];
}

bool NewModule::ValidateCounter(uint8_t counter)
{
    counter &= 0x0FU;

    if (!haveSeenValidCounter)
    {
        haveSeenValidCounter = true;
        lastRxCounter = counter;
        return true;
    }

    const uint8_t expectedCounter = static_cast<uint8_t>((lastRxCounter + 1U) & 0x0FU);
    if (counter != expectedCounter)
    {
        return false;
    }

    lastRxCounter = counter;
    return true;
}

void NewModule::DecodeCAN(int id, uint8_t* data, uint8_t dlc)
{
    if (id != NEW_MODULE_STATUS_ID || data == nullptr || dlc < 8)
    {
        return;
    }

    if (!CheckCrc(data))
    {
        return;
    }

    if (!ValidateCounter(data[6] & 0x0FU))
    {
        return;
    }

    const uint16_t rawVoltage = static_cast<uint16_t>(data[0]) |
                                (static_cast<uint16_t>(data[1]) << 8);
    measuredVoltage = rawVoltage / 10.0f;

    const uint16_t rawCurrent = static_cast<uint16_t>(data[2]) |
                                (static_cast<uint16_t>(data[3]) << 8);
    measuredCurrent = (static_cast<int32_t>(rawCurrent) - 5000) / 10.0f;

    faultBits = data[4];
    rxTimeoutTicks = NEW_MODULE_RX_TIMEOUT_TICKS_100MS;
}

void NewModule::PublishParams(bool dataValid)
{
    Param::SetInt(Param::new_module_data_valid, dataValid ? 1 : 0);
    Param::SetFloat(Param::new_module_voltage, measuredVoltage);
    Param::SetFloat(Param::new_module_current, measuredCurrent);
    Param::SetInt(Param::new_module_fault_bits, faultBits);
}

void NewModule::SendStatus()
{
    if (!can)
    {
        return;
    }

    uint8_t bytes[8] = {0};
    bytes[0] = static_cast<uint8_t>(Param::GetInt(Param::some_project_state));
    bytes[6] = txCounter & 0x0FU;

    uint32_t crcBuf[2] = {0, 0};
    memcpy(crcBuf, bytes, sizeof(crcBuf));
    crcBuf[1] &= 0x00FFFFFF;
    crc_reset();
    bytes[7] = static_cast<uint8_t>(crc_calculate_block(crcBuf, 2) & 0xFFU);

    can->Send(NEW_MODULE_COMMAND_ID, bytes, 8);
    txCounter = static_cast<uint8_t>((txCounter + 1U) & 0x0FU);
}

void NewModule::Task100Ms()
{
    if (rxTimeoutTicks > 0)
    {
        rxTimeoutTicks--;
    }

    const bool timeout = rxTimeoutTicks == 0;
    const bool fault = faultBits != 0;
    const bool dataValid = !timeout && !fault;

    if (timeout)
    {
        ErrorMessage::Post(ERR_NEW_MODULE_TIMEOUT);
    }
    if (fault)
    {
        ErrorMessage::Post(ERR_NEW_MODULE_FAULT);
    }

    PublishParams(dataValid);
    SendStatus();
}
```

## Parameter Rules
Add parameters and display values in `include/param_prj.h`.

```cpp
PARAM_ENTRY(CAT_SETUP, new_module_can, CAN_DEV, 0, 1, 1, 168)
VALUE_ENTRY(new_module_voltage, "V", 2339)
VALUE_ENTRY(new_module_current, "A", 2340)
VALUE_ENTRY(new_module_fault_bits, NEW_MODULE_FAULT_FLAGS, 2341)
VALUE_ENTRY(new_module_data_valid, YESNO, 2342)
```

Rules:
- Never reuse an old nonzero parameter/value ID.
- Increase the `Next param id` and `Next value Id` comments.
- `PARAM_ENTRY` is saveable and has min/max/default/id.
- `VALUE_ENTRY` is runtime/display-only and has unit/id.
- Keep entries grouped by category and module.
- Use enum strings such as `"0=Off, 1=On"` for dropdown/decoded display values.
- Publish module state through `Param::SetInt()` and `Param::SetFloat()`.
- Read user/config inputs through `Param::GetInt()` and `Param::GetFloat()`.

## Error Rules
Add project errors in `include/errormessage_prj.h`.

```cpp
ERROR_MESSAGE_ENTRY(NEW_MODULE_TIMEOUT, ERROR_DISPLAY)
ERROR_MESSAGE_ENTRY(NEW_MODULE_FAULT, ERROR_DISPLAY)
```

Post errors from periodic tasks or decode logic:

```cpp
ErrorMessage::Post(ERR_NEW_MODULE_TIMEOUT);
```

Use errors for actionable faults, timeouts, protocol failures, or unsafe hardware states. Avoid posting an error every time an optional frame is absent unless the module requires it for safety.

## CAN Rules
- `SetCanInterface()` owns RX registration with `can->RegisterUserMessage(id)`.
- Accept `nullptr`; clear local CAN state and publish safe defaults if needed.
- `DecodeCAN()` must return quickly for unknown IDs, null data, bad DLC, bad CRC, or bad counter.
- Use explicit little-endian decode:
  ```cpp
  uint16_t raw = static_cast<uint16_t>(data[0]) |
                 (static_cast<uint16_t>(data[1]) << 8);
  ```
- Use `static_cast` when narrowing or changing signedness.
- For CRC used in this project, copy 8 bytes into `uint32_t buf[2]`, clear byte 7 via `buf[1] &= 0x00FFFFFF`, call `crc_reset()`, then `crc_calculate_block(buf, 2) & 0xFF`.
- Use a 4-bit rolling counter with `& 0x0F` when the peer protocol has a nibble counter.
- For transmit messages, use `uint8_t bytes[8] = {0};`, fill only defined fields, then `can->Send(id, bytes, dlc)`.

## Scheduler Rules
- Put timeout decrement and parameter publishing in `Task100Ms()` unless faster reaction is needed.
- Put debounce and direct output control in `Task10Ms()`.
- Put software PWM and very short timing loops in `Task1Ms()`.
- Express timeout macros in task ticks, for example `NEW_MODULE_RX_TIMEOUT_TICKS_100MS 5`.
- Do not allocate dynamically, block, sleep, print, or do heavy loops inside tasks.

## Hardware IO Rules
- Use `DigIo::name.Get()`, `.Set()`, `.Clear()`, `.Toggle()` for digital IO.
- Use `AnaIn::name.Get()` for analog readings.
- Publish raw and scaled values when both are useful for calibration.
- Document active-low outputs at the point of use:
  ```cpp
  DigIo::heater_contactor_out.Clear(); // OFF
  DigIo::tesla_coolant_pump_out.Clear(); // ON, active low
  ```

## Build Integration
For a `.cpp` module:
- Add `src/new_module.cpp`.
- Add `include/new_module.h`.
- Add `new_module.o` to `OBJSL` in `Makefile`.
- Add `#include "new_module.h"` to `src/main.cpp`.
- Add a static instance and task calls in `main.cpp`.

Header-only modules do not need a `Makefile` object, but keep them small. If the module has complex decode/control logic, prefer `.cpp`.

## Testing Guidance
- If logic can be tested on host, add or extend tests under `test/`.
- For decode tests, feed raw frames and assert `Param` outputs.
- For timeout tests, call the task repeatedly and assert fault/data-valid behavior.
- For CAN transmit tests, use stubs to assert ID, DLC, bytes, CRC, and counter.
- Always run:
  ```bash
  make
  cd test && make
  ```
  when toolchain and submodules are available.

## Coding Style
- C++11 only; compiler flags disable RTTI and exceptions.
- No dynamic allocation for normal module behavior.
- Prefer private cached state plus explicit `Param` publishing.
- Use `nullptr` in C++ files.
- Keep constants as `#define` near the top of the module, matching existing style.
- Indentation is mostly 4 spaces in modules, 3 spaces in `main.cpp`.
- Keep comments useful: protocol byte meaning, units, scaling, active-low hardware, timeout math, and safety rationale.
- Use ASCII in new files unless an existing protocol name or unit requires otherwise.

## Module Creation Checklist
- [ ] Header guard and includes added.
- [ ] Class has `SetCanInterface`, needed `DecodeCAN`, and needed `Task*Ms` methods.
- [ ] CAN RX IDs registered in `SetCanInterface`.
- [ ] Decoder validates ID, pointer, DLC, CRC/counter if applicable.
- [ ] Private state defaults to safe values.
- [ ] Timeout/fault/data-valid behavior implemented.
- [ ] Params added with unique IDs and category.
- [ ] Errors added if needed.
- [ ] `main.cpp` include, static instance, callback fan-out, scheduler calls, and `Param::Change()` cases added.
- [ ] `Makefile` `OBJSL` updated for `.cpp`.
- [ ] Build/test command run or documented if unavailable.

