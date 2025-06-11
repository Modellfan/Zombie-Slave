#ifndef EPS_H
#define EPS_H

#include "params.h"
#include "hwdefs.h"
#include "digio.h"
#include "lvdu.h"

class EPS
{
private:
    uint16_t delayCounter = 0;     // counts 100ms steps
    bool epsActive = false;
    VehicleState lastState = STATE_SLEEP;

public:
    EPS() {}

    void Task100Ms()
    {
        VehicleState state = static_cast<VehicleState>(Param::GetInt(Param::LVDU_vehicle_state));

        // Detect state change
        if (state != lastState)
        {
            delayCounter = 0;
            epsActive = false;
            DigIo::servo_pump_out.Set(); // OFF (active low)
            lastState = state;
        }

        if (state == STATE_READY)
        {
            uint16_t delaySteps = Param::GetInt(Param::eps_startup_delay) / 100; // convert ms -> 100ms steps
            if (delayCounter < delaySteps)
            {
                delayCounter++;
            }
            if (!epsActive && delayCounter >= delaySteps)
            {
                epsActive = true;
                DigIo::servo_pump_out.Clear(); // ON (active low)
            }
        }
        else
        {
            epsActive = false;
            DigIo::servo_pump_out.Set(); // OFF
        }

        // Update parameters
        Param::SetInt(Param::eps_ignition_out, epsActive ? 1 : 0);
        Param::SetInt(Param::eps_startup_in, epsActive ? 1 : 0);
    }
};

#endif // EPS_H
