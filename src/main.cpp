/*
 * This file is part of the stm32-template project.
 *
 * Copyright (C) 2020 Johannes Huebner <dev@johanneshuebner.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
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
#include "my_math.h"
#include "errormessage.h"
#include "printf.h"
#include "stm32scheduler.h"
#include "terminalcommands.h"
#include "tesla_valve.h"
#include "tesla_coolant_pump.h"
#include "dcdc.h"
#include "TeslaDCDC.h"
#include "bms.h"
#include "teensyBMS.h"
#include "heater.h"
#include "vacuum_pump.h"
#include "lvdu.h"
#include "eps.h"

#define PRINT_JSON 0

static Stm32Scheduler *scheduler;
static CanHardware *canInterface[3];
static CanMap *canMap;
static TeslaCoolantPump coolantPump;
static TeslaValve teslaValve;
static TeslaDCDC DCDCTesla;
static TeensyBMS teensyBms;
static Heater heater;
static VacuumPump vacuumPump;
static LVDU lvdu;
static EPS eps;

// Whenever the user clears mapped can messages or changes the
// CAN interface of a device, this will be called by the CanHardware module
static void SetCanFilters()
{
   CanHardware *dcdc_can = canInterface[Param::GetInt(Param::dcdc_can)];
   CanHardware *bms_can = canInterface[Param::GetInt(Param::BMS_CAN)];

   DCDCTesla.SetCanInterface(dcdc_can);
   teensyBms.SetCanInterface(bms_can);

   canInterface[0]->RegisterUserMessage(0x601); // CanSDO
   canInterface[1]->RegisterUserMessage(0x601); // CanSDO
}

static bool CanCallback(uint32_t id, uint32_t data[2], uint8_t dlc) // This is where we go when a defined CAN message is received.
{
   dlc = dlc;
   DCDCTesla.DecodeCAN(id, (uint8_t *)data);
   teensyBms.DecodeCAN(id, (uint8_t *)data);
   return false;
}

// sample 100ms task
static void Ms100Task(void)
{
   // The following call toggles the LED output, so every 100ms
   // The LED changes from on to off and back.
   // Other calls:
   // DigIo::led_out.Set(); //turns LED on
   // DigIo::led_out.Clear(); //turns LED off
   // For every entry in digio_prj.h there is a member in DigIo
   // DigIo::led_out.Toggle();
   // The boot loader enables the watchdog, we have to reset it
   // at least every 2s or otherwise the controller is hard reset.
   iwdg_reset();
   // Calculate CPU load. Don't be surprised if it is zero.
   float cpuLoad = scheduler->GetCpuLoad();
   // This sets a fixed point value WITHOUT calling the parm_Change() function
   Param::SetFloat(Param::cpuload, cpuLoad / 10);

   // If we chose to send CAN messages every 100 ms, do this here.
   if (Param::GetInt(Param::canperiod) == CAN_PERIOD_100MS)
      canMap->SendAll();

   // Give calculation power to the module
   teslaValve.Task100Ms();
   coolantPump.Task100Ms();
   DCDCTesla.Task100Ms();
   teensyBms.Task100Ms();
   lvdu.Task100Ms();
   eps.Task100Ms();
}

// sample 10 ms task
static void Ms10Task(void)
{
   // Set timestamp of error message
   ErrorMessage::SetTime(rtc_get_counter_val());

   // Param::SetInt(Param::ignition_drive_in, DigIo::ignition_drive_input_pin.Get());

   // if (DigIo::test_in.Get())
   // {
   //    // Post a test error message when our test input is high
   //   ErrorMessage::Post(ERR_TESTERROR);
   // }

   // If we chose to send CAN messages every 10 ms, do this here.
   if (Param::GetInt(Param::canperiod) == CAN_PERIOD_10MS)
      canMap->SendAll();

   heater.Task10Ms();
   vacuumPump.Task10Ms();
}

// sample 1 ms task
static void Ms1Task(void)
{
   coolantPump.Task1Ms();
}

/** This function is called when the user changes a parameter */
void Param::Change(Param::PARAM_NUM paramNum)
{
   switch (paramNum)
   {
   case Param::BMS_CAN:
      SetCanFilters(); // Re-assign CAN interface to TeensyBMS
      break;

   case Param::dcdc_can:
      SetCanFilters(); // Re-assign CAN interface to TeensyBMS
      break;

   default:
      // Handle general parameter changes here. Add paramNum labels for handling specific parameters
      break;
   }
}

// Whichever timer(s) you use for the scheduler, you have to
// implement their ISRs here and call into the respective scheduler
extern "C" void tim2_isr(void)
{
   scheduler->Run();
}

extern "C" int main(void)
{
   extern const TERM_CMD termCmds[];

   clock_setup(); // Must always come first
   rtc_setup();
   ANA_IN_CONFIGURE(ANA_IN_LIST);
   DIG_IO_CONFIGURE(DIG_IO_LIST);
   AnaIn::Start();             // Starts background ADC conversion via DMA
   write_bootloader_pininit(); // Instructs boot loader to initialize certain pins

   tim_setup();  // Sample init of a timer
   nvic_setup(); // Set up some interrupts
   parm_load();  // Load stored parameters

   // Initialize CAN1, including interrupts. Clock must be enabled in clock_setup()
   Stm32Can c(CAN1, (CanHardware::baudrates)Param::GetInt(Param::canspeed));
   Stm32Can c2(CAN2, (CanHardware::baudrates)Param::GetInt(Param::canspeed));
   FunctionPointerCallback cb(CanCallback, SetCanFilters);
   CanMap cm(&c);
   CanSdo sdo(&c, &cm);
   sdo.SetNodeId(33); // id 33 for vcu?
   canInterface[0] = &c;
   canInterface[1] = &c2;
   c.AddCallback(&cb);
   c2.AddCallback(&cb);
   TerminalCommands::SetCanMap(&cm);
   canMap = &cm;

   // This is all we need to do to set up a terminal on USART3
   Terminal t(USART3, termCmds);
   TerminalCommands::SetCanMap(canMap);

   // This will call SetCanFilters() via the Clear Callback
   canInterface[0]->ClearUserMessages();
   canInterface[1]->ClearUserMessages();

   // Up to four tasks can be added to each timer scheduler
   // AddTask takes a function pointer and a calling interval in milliseconds.
   // The longest interval is 655ms due to hardware restrictions
   // You have to enable the interrupt (int this case for TIM2) in nvic_setup()
   // There you can also configure the priority of the scheduler over other interrupts
   Stm32Scheduler s(TIM2); // We never exit main so it's ok to put it on stack
   scheduler = &s;
   s.AddTask(Ms1Task, 1);
   s.AddTask(Ms10Task, 10);
   s.AddTask(Ms100Task, 100);

   // backward compatibility, version 4 was the first to support the "stream" command
   Param::SetInt(Param::version, 4);
   Param::Change(Param::PARAM_LAST); // Call callback one for general parameter propagation

   // Now all our main() does is running the terminal
   // All other processing takes place in the scheduler or other interrupt service routines
   // The terminal has lowest priority, so even loading it down heavily will not disturb
   // our more important processing routines.
   while (1)
   {
      char c = 0;
      t.Run();
      if (sdo.GetPrintRequest() == PRINT_JSON)
      {
         TerminalCommands::PrintParamsJson(&sdo, &c);
      }
   }

   return 0;
}
