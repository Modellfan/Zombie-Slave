# Main Integration Template

## Purpose
Use this when adding or regenerating the project `main.cpp` integration layer. The main file is intentionally small: it wires hardware, CAN, terminal/SDO, scheduler tasks, and module instances. Functional behavior belongs in modules under `src/` and `include/`.

## Project Shape
- Application source lives in `src/`.
- Public module interfaces live in `include/`.
- `main.cpp` owns global/static module instances and scheduler callbacks.
- `Makefile` must list each compiled `.cpp` object in `OBJSL`.
- `param_prj.h` defines every parameter/value visible through terminal, SDO, CAN map, and GUI tooling.
- `errormessage_prj.h` defines project-specific errors posted by modules.

## Main File Responsibilities
1. Include libopencm3/libopeninv headers and project module headers.
2. Declare system objects:
   - `Stm32Scheduler *scheduler`
   - `CanHardware *canInterface[]`
   - `CanMap *canMap`
   - CAN map transmit slot counter
3. Declare functional module instances as `static`.
4. Implement CAN filter assignment in `SetCanFilters()`.
5. Implement CAN receive fan-out in `CanCallback()`.
6. Implement scheduler tasks:
   - `Ms1Task()`: high-rate software PWM or 10 ms CAN map spreading.
   - `Ms10Task()`: fast IO/control modules, error timestamp, 100 ms CAN map spreading when configured.
   - `Ms100Task()`: watchdog reset, CPU load, slow CAN/control/state modules.
7. Implement `Param::Change()` for parameters that affect runtime wiring.
8. Implement timer ISR that calls `scheduler->Run()`.
9. Initialize clocks, pins, ADC, timers, CAN, terminal, SDO, scheduler, and then run the terminal loop forever.

## Include Order Pattern
Use local style: standard/libopencm3 first, libopeninv services next, project modules last.

```cpp
#include <stdint.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/can.h>
#include <libopencm3/stm32/iwdg.h>

#include "stm32_can.h"
#include "canmap.h"
#include "cansdo.h"
#include "terminal.h"
#include "params.h"
#include "hwdefs.h"
#include "digio.h"
#include "hwinit.h"
#include "anain.h"
#include "param_save.h"
#include "errormessage.h"
#include "stm32scheduler.h"

#include "new_module.h"
```

## Main Skeleton
Replace `NewModule` and parameter names with the real module.

```cpp
// System SW components
static Stm32Scheduler *scheduler;
static CanHardware *canInterface[3];
static CanMap *canMap;
static uint8_t canMapTxSlot;

// Functional SW components
static NewModule newModule;

static void SetCanFilters()
{
   CanHardware *moduleCan = canInterface[Param::GetInt(Param::new_module_can)];

   newModule.SetCanInterface(moduleCan);

   canInterface[0]->RegisterUserMessage(0x601); // CanSDO
   canInterface[1]->RegisterUserMessage(0x601); // CanSDO
}

static bool CanCallback(uint32_t id, uint32_t data[2], uint8_t dlc)
{
   newModule.DecodeCAN(id, (uint8_t *)data, dlc);
   return false;
}

static void Ms100Task(void)
{
   iwdg_reset();
   Param::SetFloat(Param::cpuload, scheduler->GetCpuLoad() / 10);

   newModule.Task100Ms();
}

static void Ms10Task(void)
{
   ErrorMessage::SetTime(rtc_get_counter_val());

   if (Param::GetInt(Param::canperiod) == CAN_PERIOD_100MS)
   {
      canMap->SendByIndex(canMapTxSlot);
      canMapTxSlot = (canMapTxSlot + 1) % MAX_MESSAGES;
   }

   newModule.Task10Ms();
}

static void Ms1Task(void)
{
   if (Param::GetInt(Param::canperiod) == CAN_PERIOD_10MS)
   {
      canMap->SendByIndex(canMapTxSlot);
      canMapTxSlot = (canMapTxSlot + 1) % MAX_MESSAGES;
   }

   newModule.Task1Ms();
}

void Param::Change(Param::PARAM_NUM paramNum)
{
   switch (paramNum)
   {
   case Param::new_module_can:
      SetCanFilters();
      break;

   default:
      break;
   }
}

extern "C" void tim2_isr(void)
{
   scheduler->Run();
}
```

## Initialization Sequence
Keep this order unless there is a hardware reason to change it.

```cpp
extern "C" int main(void)
{
   extern const TERM_CMD termCmds[];

   clock_setup();
   rtc_setup();
   gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON,
                      AFIO_MAPR_CAN2_REMAP | AFIO_MAPR_TIM1_REMAP_FULL_REMAP);
   ANA_IN_CONFIGURE(ANA_IN_LIST);
   DIG_IO_CONFIGURE(DIG_IO_LIST);
   AnaIn::Start();
   write_bootloader_pininit();

   tim_setup();
   nvic_setup();
   parm_load();

   Stm32Can c(CAN1, (CanHardware::baudrates)Param::GetInt(Param::canspeed));
   Stm32Can c2(CAN2, (CanHardware::baudrates)Param::GetInt(Param::canspeed), true);
   FunctionPointerCallback cb(CanCallback, SetCanFilters);
   CanMap cm(&c);
   CanSdo sdo(&c, &cm);

   canInterface[0] = &c;
   canInterface[1] = &c2;
   c.AddCallback(&cb);
   c2.AddCallback(&cb);
   canMap = &cm;

   TerminalCommands::SetCanMap(canMap);
   SdoCommands::SetCanMap(canMap);
   Terminal t(USART3, termCmds);

   canInterface[0]->ClearUserMessages();
   canInterface[1]->ClearUserMessages();

   Stm32Scheduler s(TIM2);
   scheduler = &s;
   s.AddTask(Ms1Task, 1);
   s.AddTask(Ms10Task, 10);
   s.AddTask(Ms100Task, 100);

   Param::SetInt(Param::version, 4);
   Param::Change(Param::PARAM_LAST);

   while (1)
   {
      CanSdo::SdoFrame* sdoFrame = sdo.GetPendingUserspaceSdo();
      t.Run();

      if (0 != sdoFrame)
      {
         SdoCommands::ProcessStandardCommands(sdoFrame);
         sdo.SendSdoReply(sdoFrame);
      }
   }
}
```

## Adding A Module To Main
- Add the module header include.
- Add one `static ModuleName moduleName;`.
- Add CAN routing parameter in `param_prj.h` if the module uses selectable CAN.
- In `SetCanFilters()`, select the interface with `canInterface[Param::GetInt(...)]`.
- Call `moduleName.SetCanInterface(selectedCan)`.
- In `CanCallback()`, call the module decode function. Cast `uint32_t data[2]` to `uint8_t *` for byte-oriented decoders.
- Add periodic task calls at the correct rate.
- Add `Param::Change()` cases for any parameter that changes CAN interface or major wiring.
- Add `ModuleName.o` to `OBJSL` in `Makefile` if the module has a `.cpp`.

## Scheduler Guidance
- Use `Task1Ms()` only for timing-sensitive output such as software PWM.
- Use `Task10Ms()` for fast IO control, debounce, and medium-rate CAN transmissions.
- Use `Task100Ms()` for state machines, watchdog reset, timeout counters, parameter publishing, and slower CAN transmissions.
- Counters should be sized for their task period. Example: `5` in a 100 ms task is 500 ms.
- Never block or busy-wait inside scheduler tasks.
- Keep terminal work in the infinite loop; it has lowest priority.

## CAN Guidance
- Modules register their own RX IDs in `SetCanInterface()`.
- `main.cpp` should only choose which CAN device each module uses.
- Always preserve SDO registration: register `0x601` on active CAN interfaces.
- `ClearUserMessages()` triggers the clear callback, which calls `SetCanFilters()`.
- A decoder should ignore unknown IDs and invalid data, then return quickly.

## Parameter Change Guidance
Only handle parameters with runtime side effects. Typical cases:
- CAN interface selection changed.
- Hardware mode changed and cached configuration must be rebuilt.
- Calibration/setpoint changed and a module requires immediate recompute.

Most parameter reads can happen directly in periodic tasks with `Param::GetInt()` or `Param::GetFloat()`.

## Style Notes
- Use `static` globals in `main.cpp` for system and module instances.
- Keep functions file-local unless they are required by the framework (`Param::Change`, ISR, `main`).
- Use 3-space indentation in `main.cpp`, matching existing style.
- Prefer explicit comments for hardware setup, scheduler rates, and CAN routing.
- Keep business logic out of `main.cpp`; move it into module methods.

