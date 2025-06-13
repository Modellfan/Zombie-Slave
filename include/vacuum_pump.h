#ifndef VACUUM_PUMP_H
#define VACUUM_PUMP_H

#include "params.h"
#include "hwdefs.h"
#include "digio.h"
#include "lvdu.h"
#include "errormessage.h"

class VacuumPump
{
private:
    uint16_t pump_timer = 0;            // Time counter for hysteresis (counts in 10ms steps)
    uint16_t insufficient_timer = 0;    // Time counter for insufficient vacuum warning
    bool pump_state = false;            // Current pump state (ON/OFF)

public:
    /** Default constructor */
    VacuumPump() {}

    /** Task to be executed every 10ms */
    void Task10Ms()
    {
        /*
         *  Start --> [Read vacuum_sensor_in] --> (Vacuum OK?)
         *                    | Yes |                   | No |
         *          [Start Pump Timer]           [Turn ON Pump]
         *                    |                         |
         *        (Exceeded Hysteresis?)         [Keep Pump ON]
         *                    | Yes |                 | No |
         *          [Turn OFF Pump]                 [Continue]
         *                    |
         *          (Vacuum Insufficient Too Long?)
         *                    | Yes |     | No |
         *       [Set Warning]     [Continue]
         *                    |
         *      [Update Parameters] --> End
         */

        // Read vacuum sensor (0 = No Vacuum, 1 = Vacuum OK)
        bool vacuum_ok = !(DigIo::vacuum_sensor_in.Get());
        Param::SetInt(Param::vacuum_sensor, vacuum_ok ? 1 : 0); // 1 = ON, 0 = OFF

        // Only operate the pump while the vehicle is in READY, DRIVE or LIMP_HOME state
        VehicleState vehicleState = static_cast<VehicleState>(Param::GetInt(Param::LVDU_vehicle_state));
        if (vehicleState != STATE_READY && vehicleState != STATE_DRIVE && vehicleState != STATE_LIMP_HOME)
        {
            pump_state = false;
            pump_timer = 0;
            insufficient_timer = 0;
            DigIo::vacuum_pump_out.Clear(); // OFF (Active Low)
            Param::SetInt(Param::vacuum_pump_insufficient, 0);
            Param::SetInt(Param::vacuum_pump_out, 0);
            return;
        }

        // Read hysteresis and warning delay parameters (convert from ms to 10ms steps)
        uint16_t hysteresis_time = Param::GetInt(Param::vacuum_hysteresis) / 10;
        uint16_t warning_delay = Param::GetInt(Param::vacuum_warning_delay) / 10;

        // Handle vacuum pump state
        if (!vacuum_ok) // Vacuum NOT OK -> Turn ON pump immediately
        {
            pump_state = true;
            pump_timer = 0; // Reset hysteresis timer
            DigIo::vacuum_pump_out.Set(); // ON (Active Low)
        }
        else // Vacuum OK -> Start pump OFF timer
        {
            if (pump_state)
            {
                pump_timer++; // Increment hysteresis timer (each step is 10ms)
                if (pump_timer >= hysteresis_time)
                {
                    pump_state = false;
                    DigIo::vacuum_pump_out.Clear(); // OFF
                    pump_timer = 0; // Reset counter
                }
            }
        }

        // Track insufficient vacuum duration
        if (!vacuum_ok)
        {
            insufficient_timer++; // Count 10ms steps
            if (insufficient_timer >= warning_delay)
            {
                // Set Insufficient Vacuum Warning
                Param::SetInt(Param::vacuum_pump_insufficient, 1);
                ErrorMessage::Post(ERR_VACUUM_INSUFFICIENT);
            }
        }
        else
        {
            // Reset warning when vacuum is OK
            insufficient_timer = 0;
            Param::SetInt(Param::vacuum_pump_insufficient, 0);
        }

        // **Update ECU Parameters**
        Param::SetInt(Param::vacuum_pump_out, pump_state ? 1 : 0); // 1 = ON, 0 = OFF
        
    }
};

#endif // VACUUM_PUMP_H
