#ifndef EPS_H
#define EPS_H

#include "params.h"
#include "hwdefs.h"
#include "digio.h"
#include "lvdu.h"
#include "errormessage.h"

class EPS
{
private:
    uint16_t spoolupCounter = 0; // counts 100ms steps
    bool ignitionActive = false;
    bool spoolupActive = false;
    _eps_states epsState = EPS_OFF;
    VehicleState lastState = STATE_SLEEP;

public:
    EPS() {}

    void Task100Ms()
    {
        VehicleState state = static_cast<VehicleState>(Param::GetInt(Param::LVDU_vehicle_state));
        bool dcdcFault = Param::GetInt(Param::dcdc_fault_any);

        // Handle state transitions
        if (state != lastState)
        {
            spoolupCounter = 0;
            spoolupActive = false;
            ignitionActive = false;

            if (state == STATE_READY)
            {
                if (!dcdcFault)
                {
                    ignitionActive = true;
                    epsState = EPS_ON;
                    DigIo::eps_ignition_on_out.Set();
                }
                else
                {
                    epsState = EPS_FAULT;
                    DigIo::eps_ignition_on_out.Clear();
                    DigIo::eps_quick_spoolup_out.Clear();
                    ErrorMessage::Post(ERR_EPS_STARTUP_DCDC_FAULT);
                }
            }
            lastState = state;
        }

        // Keep outputs high in READY/DRIVE/LIMP_HOME
        if ((state == STATE_READY || state == STATE_DRIVE || state == STATE_LIMP_HOME) && epsState == EPS_ON)
        {
            if (ignitionActive && !spoolupActive)
            {
                uint16_t delaySteps = Param::GetInt(Param::eps_spoolup_delay) / 100; // ms -> 100ms
                if (spoolupCounter < delaySteps)
                    spoolupCounter++;
                if (spoolupCounter >= delaySteps)
                {
                    spoolupActive = true;
                    DigIo::eps_quick_spoolup_out.Set();
                }
            }

            if (ignitionActive)
                DigIo::eps_ignition_on_out.Set();
            if (spoolupActive)
                DigIo::eps_quick_spoolup_out.Set();
        }
        else
        {
            ignitionActive = false;
            spoolupActive = false;
            if (state != STATE_READY && state != STATE_DRIVE && state != STATE_LIMP_HOME)
                epsState = EPS_OFF;
            DigIo::eps_ignition_on_out.Clear();
            DigIo::eps_quick_spoolup_out.Clear();
        }

        Param::SetInt(Param::eps_ignition_out, ignitionActive ? 1 : 0);
        Param::SetInt(Param::eps_startup_in, spoolupActive ? 1 : 0);
        Param::SetInt(Param::eps_state, epsState);
    }
};

#endif // EPS_H
