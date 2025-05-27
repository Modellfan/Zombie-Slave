#ifndef LVDU_H
#define LVDU_H

#include "params.h"
#include "hwdefs.h"
#include "digio.h"

#define VOLTAGE_DIVIDER_RATIO 0.0059f

enum VehicleState
{
    STATE_SLEEP = 0,
    STATE_STANDBY,
    STATE_READY,
    STATE_CONDITIONING,
    STATE_DRIVE,
    STATE_CHARGE,
    STATE_ERROR,
    STATE_LIMP_HOME
};

class LVDU
{
private:
    VehicleState state = STATE_SLEEP;
    VehicleState lastState = STATE_SLEEP;

    // Diagnose
    bool diagnosePending = false;
    uint16_t diagnoseTimer = 0;
    static constexpr uint16_t diagnoseDelaySteps = 20; // 200ms

    // Force Standby/Sleep
    bool forceStandbyActive = false;
    bool forceSleepActive = false;
    uint16_t forceStandbyTimer = 0;
    uint16_t forceSleepTimer = 0;
    static constexpr uint16_t forceDelaySteps = 2000 / 10; // 20s

    // Interne Flags
    bool ignitionOn = false;
    float voltage12V = 13.2f;                    // Cached analog read
    bool is12VTooLow = false;                    // Cached threshold comparison
    bool chargerPlugged = false;                 // TODO: Add real charger detection via CAN
    bool remotePreconditioningRequested = false; // TODO: Add flag/CAN/schedule check via CAN
    bool thermalTaskCompleted = false;           // TODO: Implement or simulate via CAN
    bool criticalFault = false;                  // TODO: Detect via system/BMS via CAN
    bool degradedFault = false;                  // TODO: Detect via system/BMS via CAN

public:
    LVDU() {}

    void Task10Ms()
    {
        UpdateInputs();
        UpdateState();
        UpdateParams();
    }

private:
    void UpdateInputs()
    {
        // Read digital inputs
        ignitionOn = DigIo::ignition_in.Get();
        bool readySafety = DigIo::ready_safety_in.Get();

        // Read 12V analog input and apply voltage divider ratio
        voltage12V = AnaIn::dc_power_supply.Get() * VOLTAGE_DIVIDER_RATIO;

        // Threshold evaluation (Is12VTooLow logic)
        is12VTooLow = voltage12V < Param::GetFloat(Param::LVDU_12v_low_threshold);

        // Update runtime value parameters
        Param::SetInt(Param::LVDU_ignition_in, ignitionOn ? 1 : 0);
        Param::SetInt(Param::LVDU_ready_safety_in, readySafety ? 1 : 0);
        Param::SetFloat(Param::LVDU_12v_battery_voltage, voltage12V);
        Param::SetInt(Param::LVDU_12v_too_low, is12VTooLow ? 1 : 0);
    }

    void UpdateState()
    {
        switch (state)
        {
        case STATE_SLEEP:
            if (ignitionOn)
            {
                TransitionTo(STATE_STANDBY); // Light wake
            }
            else if (chargerPlugged)
            {
                TransitionTo(STATE_STANDBY);
            }
            else if (remotePreconditioningRequested)
            {
                TransitionTo(STATE_STANDBY);
            }
            break;

        case STATE_STANDBY:
            if (ignitionOn)
            {
                TransitionTo(STATE_READY); // HV activation path
            }
            else if (chargerPlugged)
            {
                TransitionTo(STATE_CHARGE);
            }
            else
            {
                // Optional: Timeout or inactivity could return to sleep
                TransitionTo(STATE_SLEEP);
            }
            break;

        case STATE_READY:
            if (!ignition)
            {
                TransitionTo(STATE_CONDITIONING); // Shutdown & cool
            }
            else if (IsDriveRequestReceived())
            {
                TransitionTo(STATE_DRIVE);
            }
            else if (chargerPlugged)
            {
                TransitionTo(STATE_CHARGE);
            }
            break;

        case STATE_CONDITIONING:
            if (ignition)
            {
                TransitionTo(STATE_READY); // Resume full control
            }
            else if (chargerPlugged)
            {
                TransitionTo(STATE_CHARGE);
            }
            else if (thermalTaskCompleted)
            {
                TransitionTo(STATE_SLEEP); // Cooling complete
            }
            else if (criticalFault)
            {
                TransitionTo(STATE_ERROR);
            }
            break;

        case STATE_DRIVE:
            if (!ignition)
            {
                TransitionTo(STATE_CONDITIONING);
            }
            else if (criticalFault)
            {
                TransitionTo(STATE_ERROR);
            }
            else if (degradedFault)
            {
                TransitionTo(STATE_LIMP_HOME);
            }
            else if (chargerPlugged)
            {
                TransitionTo(STATE_CHARGE); // Ignition off + plug detected
            }
            break;

        case STATE_CHARGE:
            if (!chargerPlugged)
            {
                TransitionTo(STATE_CONDITIONING);
            }
            else if (criticalFault)
            {
                TransitionTo(STATE_ERROR);
            }
            break;

        case STATE_LIMP_HOME:
            if (!ignition)
            {
                TransitionTo(STATE_CONDITIONING);
            }
            else if (criticalFault)
            {
                TransitionTo(STATE_ERROR);
            }
            break;

        case STATE_ERROR:
            if (!ignition)
            {
                TransitionTo(STATE_SLEEP); // After 12V reset
            }
            break;
        }

        // Force Standby if HV too low
        if (!forceStandbyActive && IsHVTooLow())
        {
            forceStandbyActive = true;
            forceStandbyTimer = forceDelaySteps;
        }
        if (forceStandbyActive && --forceStandbyTimer == 0)
        {
            forceStandbyActive = false;
            TransitionTo(STATE_STANDBY);
        }

        // Force Sleep if 12V too low
        if (!forceSleepActive && is12VTooLow)
        {
            forceSleepActive = true;
            forceSleepTimer = forceDelaySteps;
        }
        if (forceSleepActive && --forceSleepTimer == 0)
        {
            forceSleepActive = false;
            TransitionTo(STATE_SLEEP);
        }
    }

    void TransitionTo(VehicleState newState)
    {
        lastState = state;
        state = newState;

        // READY → CONDITIONING: Diagnose starten
        if (lastState == STATE_READY && newState == STATE_CONDITIONING)
        {
            diagnosePending = true;
            diagnoseTimer = diagnoseDelaySteps;
        }
    }

    // void HandleReadyDiagnosis()
    // {
    //     if (diagnosePending)
    //     {
    //         if (diagnoseTimer > 0)
    //         {
    //             diagnoseTimer--;
    //             DigIo::ready_out.Set(); // Relais bleibt aktiv
    //             if (!DigIo::ready_safety_in.Get())
    //             {
    //                 Param::SetInt(Param::lasterr, 1); // Beispielcode
    //             }
    //             return; // während Diagnose keine anderen Änderungen
    //         }
    //         else
    //         {
    //             DigIo::ready_out.Clear(); // jetzt abschalten
    //             diagnosePending = false;
    //         }
    //     }
    // }

    // void UpdateOutputs()
    // {
    //     switch (state)
    //     {
    //     case STATE_SLEEP:
    //     case STATE_STANDBY:
    //     case STATE_ERROR:
    //         DigIo::vcu_out.Clear();
    //         DigIo::condition_out.Clear();
    //         DigIo::ready_out.Clear();
    //         break;

    //     case STATE_CONDITIONING:
    //         DigIo::vcu_out.Set();
    //         DigIo::condition_out.Set();
    //         if (!diagnosePending)
    //             DigIo::ready_out.Clear();
    //         break;

    //     case STATE_READY:
    //     case STATE_DRIVE:
    //     case STATE_LIMP_HOME:
    //         DigIo::vcu_out.Set();
    //         DigIo::condition_out.Set();
    //         DigIo::ready_out.Set();
    //         break;

    //     case STATE_CHARGE:
    //         DigIo::vcu_out.Clear();
    //         DigIo::condition_out.Clear();
    //         DigIo::ready_out.Clear();
    //         break;
    //     }
    // }

    void UpdateOutputs()
    {
        switch (state)
        {
        case STATE_SLEEP:
            DigIo::vcu_out.Clear();
            DigIo::condition_out.Clear();
            DigIo::ready_out.Clear();
        case STATE_ERROR:
            DigIo::vcu_out.Clear();
            DigIo::condition_out.Clear();
            DigIo::ready_out.Clear();
            break;

        case STATE_STANDBY:
            DigIo::vcu_out.Set();
            DigIo::condition_out.Clear();
            DigIo::ready_out.Clear();
        case STATE_CHARGE:
            DigIo::vcu_out.Set();
            DigIo::condition_out.Set();
            DigIo::ready_out.Set();
            break;

        case STATE_CONDITIONING:
            DigIo::vcu_out.Set();
            DigIo::condition_out.Set();
            if (!diagnosePending)
                DigIo::ready_out.Clear();
            break;

        case STATE_READY:
            DigIo::vcu_out.Set();
            DigIo::condition_out.Set();
            DigIo::ready_out.Set();
        case STATE_DRIVE:
            DigIo::vcu_out.Set();
            DigIo::condition_out.Set();
            DigIo::ready_out.Set();
        case STATE_LIMP_HOME:
            DigIo::vcu_out.Set();
            DigIo::condition_out.Set();
            DigIo::ready_out.Set();
            break;
        }
    }

    void UpdateParams()
    {
        Param::SetInt(Param::LVDU_vehicle_state, static_cast<int>(state));
        Param::SetInt(Param::LVDU_last_vehicle_state, static_cast<int>(lastState));

        Param::SetInt(Param::LVDU_diagnose_pending, diagnosePending ? 1 : 0);
        Param::SetInt(Param::LVDU_diagnose_timer, diagnoseTimer * 10); // steps → ms

        Param::SetInt(Param::LVDU_force_standby_active, forceStandbyActive ? 1 : 0);
        Param::SetInt(Param::LVDU_force_standby_timer, forceStandbyTimer * 10);

        Param::SetInt(Param::LVDU_force_sleep_active, forceSleepActive ? 1 : 0);
        Param::SetInt(Param::LVDU_force_sleep_timer, forceSleepTimer * 10);

        Param::SetInt(Param::LVDU_vcu_out, DigIo::vcu_out.Get() ? 1 : 0);
        Param::SetInt(Param::LVDU_condition_out, DigIo::condition_out.Get() ? 1 : 0);
        Param::SetInt(Param::LVDU_ready_out, DigIo::ready_out.Get() ? 1 : 0);

        Param::SetInt(Param::LVDU_hv_too_low, IsHVTooLow() ? 1 : 0);
    }
};

#endif // LVDU_H
