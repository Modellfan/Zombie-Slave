#ifndef HEATER_H
#define HEATER_H

#include "params.h"
#include "hwdefs.h"
#include "digio.h"
#include "lvdu.h"

/*
    Heater Control Function – Logic Overview
    ========================================

    Start
     |
     v
    Check hv_comfort_functions_allowed and vehicle state?
       ├─ No  → Heater OFF
       └─ Yes → Continue
                |
                v
        Check aggregated fault flag?
           ├─ Fault present → Heater OFF
           └─ No fault → Continue
                        |
                        v
            Manual override OR flap > threshold?
               ├─ No  → Heater OFF
               └─ Yes → Continue
                            |
                            v
                    Thermal switch closed?
                       ├─ No → Reset delay counter, Heater OFF
                       └─ Yes
                            |
                            v
                       Increment delay timer
                            |
                            v
                   Delay timer ≥ heater_contactor_on_delay?
                       ├─ No → Heater OFF (keep counting)
                       └─ Yes → Close heater contactor (active low),
                                Set heater_active = true

    Conditions causing heater OFF:
      - hv_comfort_functions_allowed == 0
      - LVDU state not READY/CONDITIONING/DRIVE/CHARGE/LIMP_HOME
      - Any fault active
      - Flap below threshold and no manual override
      - Thermal switch open
*/


// Constants
#define CONTACTOR_FAULT_DEBOUNCE_COUNT         2      // 2 x 10ms = 20ms

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
        VehicleState vehicle_state = static_cast<VehicleState>(Param::GetInt(Param::LVDU_vehicle_state));
        bool lvdu_ok = vehicle_state == STATE_READY || vehicle_state == STATE_CONDITIONING ||
                       vehicle_state == STATE_DRIVE || vehicle_state == STATE_CHARGE ||
                       vehicle_state == STATE_LIMP_HOME;
        bool thermal_closed   = DigIo::heater_thermal_switch_in.Get(); // High = closed
        bool contactor_feedback = (DigIo::heater_contactor_feedback_in.Get() == 1); 
        bool contactor_out      = (DigIo::heater_contactor_out.Get() == 1);         

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

        if (comfort_allowed && lvdu_ok && !fault_present && heater_should_run)
        {
            if (thermal_closed)
            {
                if (contactor_on_delay_timer < Param::GetInt(Param::heater_contactor_on_delay) / 10)
                {
                    contactor_on_delay_timer++;
                }

                if (contactor_on_delay_timer >= Param::GetInt(Param::heater_contactor_on_delay) / 10)
                {
                    DigIo::heater_contactor_out.Set(); //on
                    heater_active = true;
                }
                else
                {
                    DigIo::heater_contactor_out.Clear(); //off
                    heater_active = false;
                }
            }
            else
            {
                contactor_on_delay_timer = 0;
                DigIo::heater_contactor_out.Clear(); // OFF
                heater_active = false;
            }
        }
        else
        {
            contactor_on_delay_timer = 0;
            DigIo::heater_contactor_out.Clear(); // OFF
            heater_active = false;
        }

        Param::SetInt(Param::heater_active, heater_active ? 1 : 0);
    }

private:
    void DiagnoseContactor()
    {
        bool cmd_on = (DigIo::heater_contactor_out.Get() == 1);              
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
        bool contactor_on = (DigIo::heater_contactor_out.Get() == 1); 
        bool thermal_closed = DigIo::heater_thermal_switch_in.Get();  // High = closed

        int open_timeout_steps  = Param::GetInt(Param::heater_thermal_open_timeout) * 100;  // seconds -> 10ms steps
        int close_timeout_steps = Param::GetInt(Param::heater_thermal_close_timeout) * 100; // seconds -> 10ms steps

        // Fault: Should open after contactor ON
        if (contactor_on && thermal_closed)
        {
            if (++thermal_open_timer >= open_timeout_steps)
                Param::SetInt(Param::heater_thermal_switch_does_not_open_fault, 1);
        }
        else
        {
            thermal_open_timer = 0;
        }

        // Fault: Should close after contactor OFF
        if (!contactor_on && !thermal_closed)
        {
            if (++thermal_close_timer >= close_timeout_steps)
                Param::SetInt(Param::heater_thermal_switch_overheat_fault, 1);
        }
        else
        {
            thermal_close_timer = 0;
        }
    }
};

#endif // HEATER_H
