#ifndef HEATER_H
#define HEATER_H

#include "params.h"
#include "hwdefs.h"
#include "digio.h"

/*
    Heater Control Function – Logic Overview
    ========================================

           +----------------------------+
           | hv_comfort_functions_allowed? |
           +------------+---------------+
                        |
                      [YES]
                        |
                        v
         +-------------------------------+
         | Fault present? (any fault flag) |
         +------------+------------------+
                      |
                    [NO]
                      |
                      v
         +---------------------------------------------+
         | Flap input > threshold  OR manual override? |
         +------------+------------------+
                      |
                    [YES]
                      |
                      v
         +---------------------------------------------+
         | Thermal switch closed?                      |
         +------------+------------------+
                      |
                    [YES]
                      |
                      v
     +----------------------------------------------------+
     | Start ON delay counter (in 10ms steps)             |
     | - Restart counter when thermal switch closes       |
     | - Only count up while thermal switch remains closed|
     +----------------------------------------------------+
                      |
                      v
         +-------------------------------+
         | Delay counter >= threshold?   |
         +------------+------------------+
                      |
                    [YES]
                      |
                      v
               +-------------------+
               | Close contactor   |
               | (active low)      |
               +-------------------+

    Else:
      - Reset delay counter
      - Set heater output OFF
      - Do NOT close contactor

    Conditions for heater OFF:
      - hv_comfort_functions_allowed == 0
      - Any fault active
      - No flap input & no manual override
      - Thermal switch open
*/


// Constants
#define CONTACTOR_FAULT_DEBOUNCE_COUNT         2      // 2 x 10ms = 20ms
#define HEATER_THERMAL_OPEN_TIMEOUT_MS         2000
#define HEATER_THERMAL_CLOSE_TIMEOUT_MS        5000

#define HEATER_THERMAL_OPEN_TIMEOUT_STEPS      (HEATER_THERMAL_OPEN_TIMEOUT_MS / 10)
#define HEATER_THERMAL_CLOSE_TIMEOUT_STEPS     (HEATER_THERMAL_CLOSE_TIMEOUT_MS / 10)

class Heater
{
private:
    bool heater_active = false;
    bool thermal_switch_boot_checked = false;

    uint8_t contactor_fault_timer_on = 0;
    uint8_t contactor_fault_timer_off = 0;
    uint16_t thermal_open_timer = 0;
    uint16_t thermal_close_timer = 0;
    uint16_t contactor_on_delay_timer = 0;

    bool thermal_switch_was_open = true;

public:
    Heater() {}

    void CheckThermalSwitchBootFault()
    {
        if (!thermal_switch_boot_checked)
        {
            bool thermal_ok = DigIo::heater_thermal_switch_in.Get(); // High = OK
            if (!thermal_ok)
                Param::SetInt(Param::heater_thermal_switch_boot_fault, 1);
            thermal_switch_boot_checked = true;
        }
    }

    void Task10Ms()
    {
        CheckThermalSwitchBootFault();

        DiagnoseContactor();
        DiagnoseThermalSwitch();

        // Read control inputs
        int flap_signal       = Param::GetInt(Param::valve_in_raw);
        int flap_threshold    = Param::GetInt(Param::heater_flap_threshold);
        bool manual_override  = Param::GetInt(Param::heater_active_manual);
        bool comfort_allowed  = Param::GetInt(Param::hv_comfort_functions_allowed);
        bool thermal_closed   = DigIo::heater_thermal_switch_in.Get(); // High = closed
        bool contactor_feedback = (DigIo::heater_contactor_feedback_in.Get() == 1); 
        bool contactor_out      = (DigIo::heater_contactor_out.Get() == 0);         // active low

        // Update parameter values
        Param::SetInt(Param::heater_flap_in, flap_signal);
        Param::SetInt(Param::heater_thermal_switch_in, thermal_closed ? 1 : 0);
        Param::SetInt(Param::heater_contactor_feedback_in, contactor_feedback ? 1 : 0);
        Param::SetInt(Param::heater_contactor_out, contactor_out ? 1 : 0);

        // Aggregate fault flag
        int fault_present =
            Param::GetInt(Param::heater_thermal_switch_boot_fault) ||
            Param::GetInt(Param::heater_contactor_fault) ||
            Param::GetInt(Param::heater_thermal_switch_does_not_open_fault) ||
            Param::GetInt(Param::heater_thermal_switch_overheat_fault);

        Param::SetInt(Param::heater_fault, fault_present ? 1 : 0);

        // Main control logic
        bool heater_should_run = (manual_override || flap_signal > flap_threshold);

        // Detect thermal switch closing (rising edge)
        if (thermal_closed && thermal_switch_was_open)
        {
            contactor_on_delay_timer = 0;
        }
        thermal_switch_was_open = !thermal_closed;

        if (comfort_allowed && !fault_present && heater_should_run)
        {
            if (thermal_closed)
            {
                if (contactor_on_delay_timer < Param::GetInt(Param::heater_contactor_on_delay) / 10)
                {
                    contactor_on_delay_timer++;
                }

                if (contactor_on_delay_timer >= Param::GetInt(Param::heater_contactor_on_delay) / 10)
                {
                    DigIo::heater_contactor_out.Clear(); // ON (active low)
                    heater_active = true;
                }
                else
                {
                    DigIo::heater_contactor_out.Set(); // OFF
                    heater_active = false;
                }
            }
            else
            {
                contactor_on_delay_timer = 0;
                DigIo::heater_contactor_out.Set(); // OFF
                heater_active = false;
            }
        }
        else
        {
            contactor_on_delay_timer = 0;
            DigIo::heater_contactor_out.Set(); // OFF
            heater_active = false;
        }

        Param::SetInt(Param::heater_active, heater_active ? 1 : 0);
    }

private:
    void DiagnoseContactor()
    {
        bool cmd_on = (DigIo::heater_contactor_out.Get() == 0);              // Active low
        bool feedback_closed = (DigIo::heater_contactor_feedback_in.Get() == 1); 
        bool thermal_closed = DigIo::heater_thermal_switch_in.Get();        // High = closed

        int fault = Param::GetInt(Param::heater_contactor_fault); // Keep current state

        // Fault: Commanded ON, no feedback, but only if thermal switch is closed (i.e., power allowed)
        if (cmd_on && !feedback_closed && thermal_closed)
        {
            if (++contactor_fault_timer_on >= CONTACTOR_FAULT_DEBOUNCE_COUNT)
                fault = 1;
        }
        else
        {
            contactor_fault_timer_on = 0;
        }

        // Fault: Commanded OFF, but feedback still closed
        if (!cmd_on && feedback_closed)
        {
            if (++contactor_fault_timer_off >= CONTACTOR_FAULT_DEBOUNCE_COUNT)
                fault = 2;
        }
        else
        {
            contactor_fault_timer_off = 0;
        }

        Param::SetInt(Param::heater_contactor_fault, fault);
    }

    void DiagnoseThermalSwitch()
    {
        bool contactor_on = (DigIo::heater_contactor_out.Get() == 0); // Active low
        bool thermal_closed = DigIo::heater_thermal_switch_in.Get();  // High = closed

        // Fault: Should open after contactor ON
        if (contactor_on && thermal_closed)
        {
            if (++thermal_open_timer >= HEATER_THERMAL_OPEN_TIMEOUT_STEPS)
                Param::SetInt(Param::heater_thermal_switch_does_not_open_fault, 1);
        }
        else
        {
            thermal_open_timer = 0;
        }

        // Fault: Should close after contactor OFF
        if (!contactor_on && !thermal_closed)
        {
            if (++thermal_close_timer >= HEATER_THERMAL_CLOSE_TIMEOUT_STEPS)
                Param::SetInt(Param::heater_thermal_switch_overheat_fault, 1);
        }
        else
        {
            thermal_close_timer = 0;
        }
    }
};

#endif // HEATER_H
