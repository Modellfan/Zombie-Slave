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

public:
    EPS() {}

    void Task100Ms()
    {
        VehicleState state = static_cast<VehicleState>(Param::GetInt(Param::LVDU_vehicle_state));
        bool dcdcOk = Param::GetInt(Param::dcdc_fault_any) == 0;
        float dcdcVoltage = Param::GetFloat(Param::dcdc_output_voltage);

        bool activeState = state == STATE_READY || state == STATE_DRIVE || state == STATE_LIMP_HOME;
        bool dcdcReady = dcdcOk && dcdcVoltage > 9.0f;

        if (epsState == EPS_OFF)
        {
            // In State
            ignitionActive = false;
            spoolupActive = false;
            spoolupCounter = 0;

            // Transition
            if (activeState && dcdcReady)
                epsState = EPS_SPOOL_UP;
        }
        else if (epsState == EPS_ON)
        {
            // In State
            ignitionActive = true;
            spoolupActive = true;
            spoolupCounter = 0;

            // Transition
            if (!(activeState && dcdcReady))
                epsState = EPS_OFF;
        }
        if (epsState == EPS_SPOOL_UP)
        {
            // In State
            uint16_t delaySteps = Param::GetInt(Param::eps_spoolup_delay) / 100; // ms -> 100ms
            if (spoolupCounter < delaySteps)
                spoolupCounter++;

            ignitionActive = true;
            spoolupActive = false;

            // Transition
            if (spoolupCounter >= delaySteps)
            {
                spoolupActive = true;
                epsState = EPS_ON;
            }

            if (!(activeState && dcdcReady))
                epsState = EPS_OFF;
        }
        else // Fault
        {
        }

        if (ignitionActive)
            DigIo::eps_ignition_on_out.Set();
        else
            DigIo::eps_ignition_on_out.Clear();

        if (spoolupActive)
            DigIo::eps_quick_spoolup_out.Set();
        else
            DigIo::eps_quick_spoolup_out.Clear();

        Param::SetInt(Param::eps_ignition_out, ignitionActive ? 1 : 0);
        Param::SetInt(Param::eps_startup_out, spoolupActive ? 1 : 0);
        Param::SetInt(Param::eps_state, epsState);
    }
};

#endif // EPS_H
