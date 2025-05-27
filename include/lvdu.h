#ifndef LVDU_H
#define LVDU_H

#include "params.h"
#include "hwdefs.h"
#include "digio.h"
#include "errormessage.h"
#include "anain.h"

// Configuration macros
#define LVDU_DIAGNOSE_DELAY_STEPS 400   // 4000 ms - How long hold the relay after ignition off to check, if the relay is working 
#define LVDU_READY_DELAY_STEPS 200      // 2000 ms - How long is the hystersis, after that the ready_in shoould follow the ignition
#define LVDU_FORCE_DELAY_STEPS 2000      // 20s - How long after force should down because of low HV or LV  
#define LVDU_STANDBY_TIMEOUT_STEPS 1000 // 10s @ 10ms - How long in standby to should down
#define VOLTAGE_DIVIDER_RATIO 0.0059f //For internal 12V measurement

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
    uint16_t readySetDelayTimer = 0;

    // Force Standby/Sleep
    bool forceStandbyActive = false;
    bool forceSleepActive = false;
    uint16_t forceStandbyTimer = 0;
    uint16_t forceSleepTimer = 0;
    uint16_t standbyTimeoutCounter = 0;

    // Interne Flags
    bool ignitionOn = false;
    float voltage12V = 13.2f; // Cached analog read
    bool is12VTooLow = false; // Cached threshold comparison
    bool IsHVTooLow = false;
    bool chargerPlugged = false;                 // TODO: Add real charger detection via CAN
    bool remotePreconditioningRequested = false; // TODO: Add flag/CAN/schedule check via CAN
    bool thermalTaskCompleted = true;           // TODO: Implement or simulate via CAN
    bool criticalFault = false;                  // TODO: Detect via system/BMS via CAN
    bool degradedFault = false;                  // TODO: Detect via system/BMS via CAN
    bool driverequestrecieved = false;           // TODO: Detect via system/BMS via CAN
    bool standbyTimeout = false;

public:
    LVDU() {}

    void Task10Ms()
    {
        UpdateInputs();
        UpdateState();
        HandleReadyDiagnosis();
        UpdateOutputs();
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
    }

    void UpdateState()
    {
        switch (state)
        {
        case STATE_SLEEP:
            // Automatically transition to Standby (e.g., from wake sources)
            TransitionTo(STATE_STANDBY);
            break;

        case STATE_STANDBY:
            if (ignitionOn)
            {
                TransitionTo(STATE_READY);
            }
            else if (remotePreconditioningRequested)
            {
                TransitionTo(STATE_CONDITIONING);
            }
            else if (chargerPlugged)
            {
                TransitionTo(STATE_CHARGE);
            }
            else
            {
                if (++standbyTimeoutCounter >= LVDU_STANDBY_TIMEOUT_STEPS)
                {
                    standbyTimeout = true;
                }
            }

            if (standbyTimeout)
            {
                standbyTimeout = false;
                TransitionTo(STATE_SLEEP);
            }
            break;

        case STATE_READY:
            if (!ignitionOn)
                TransitionTo(STATE_CONDITIONING);
            else if (driverequestrecieved)
                TransitionTo(STATE_DRIVE);
            else if (chargerPlugged)
                TransitionTo(STATE_CHARGE);
            else if (criticalFault)
                TransitionTo(STATE_ERROR);
            break;

        case STATE_CONDITIONING:
            if (ignitionOn)
                TransitionTo(STATE_READY);
            else if (chargerPlugged)
                TransitionTo(STATE_CHARGE);
            else if (thermalTaskCompleted)
                TransitionTo(STATE_STANDBY);
            else if (criticalFault)
                TransitionTo(STATE_ERROR);
            break;

        case STATE_DRIVE:
            if (!ignitionOn)
                TransitionTo(STATE_CONDITIONING);
            else if (chargerPlugged)
                TransitionTo(STATE_CHARGE);
            else if (degradedFault)
                TransitionTo(STATE_LIMP_HOME);
            break;

        case STATE_CHARGE:
            if (!chargerPlugged)
                TransitionTo(STATE_CONDITIONING);
            else if (criticalFault)
                TransitionTo(STATE_ERROR);
            break;

        case STATE_ERROR:
            if (!ignitionOn)
                TransitionTo(STATE_SLEEP);
            break;

        case STATE_LIMP_HOME:
            if (!ignitionOn)
                TransitionTo(STATE_CONDITIONING);
            break;
        }

        // Check HV too low during READY or CONDITIONING
        if ((state == STATE_READY || state == STATE_CONDITIONING) && IsHVTooLow)
        {
            if (!forceStandbyActive)
            {
                forceStandbyActive = true;
                forceStandbyTimer = LVDU_FORCE_DELAY_STEPS;
            }
        }
        else
        {
            forceStandbyActive = false;
            forceStandbyTimer = 0;
        }

        if (forceStandbyActive && --forceStandbyTimer == 0)
        {
            forceStandbyActive = false;
            TransitionTo(STATE_STANDBY);
        }

        // Check LV too low during STANDBY or ERROR
        if ((state == STATE_STANDBY || state == STATE_ERROR) && is12VTooLow)
        {
            if (!forceSleepActive)
            {
                forceSleepActive = true;
                forceSleepTimer = LVDU_FORCE_DELAY_STEPS;
            }
        }
        else
        {
            forceSleepActive = false;
            forceSleepTimer = 0;
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
    
        // Reset standby timeout counter if leaving STANDBY
        if (lastState == STATE_STANDBY)
        {
            standbyTimeout = false;
            standbyTimeoutCounter = 0;
        }
    
        // READY → CONDITIONING: Start diagnosis
        if (lastState == STATE_READY && newState == STATE_CONDITIONING)
        {
            diagnosePending = true;
            diagnoseTimer = LVDU_DIAGNOSE_DELAY_STEPS;
        }
    }
    

    void HandleReadyDiagnosis()
    {

        // Case 1: ignition ON → ready_safety_in must follow within 100ms
        if (ignitionOn)
        {
            if (readySetDelayTimer < LVDU_READY_DELAY_STEPS)
            {
                readySetDelayTimer++;

                // If ready doesn't follow in time
                if (readySetDelayTimer == LVDU_READY_DELAY_STEPS && !DigIo::ready_safety_in.Get())
                {
                    ErrorMessage::Post(ERR_READY_NOT_SET_ON_IGNITION);
                }
            }
            return; // Skip rest if ignition is on
        }
        else
        {
            // Reset delay tracking
            readySetDelayTimer = 0;
        }

        // Case 2: Diagnose phase (e.g., READY → CONDITIONING)
        if (diagnosePending)
        {
            if (diagnoseTimer > 0)
            {
                diagnoseTimer--;

                DigIo::ready_out.Set(); // Keep ready ON during diagnose

                // Error: ready_safety_in dropped during diagnose
                if (!DigIo::ready_safety_in.Get())
                {
                    ErrorMessage::Post(ERR_READY_DROPPED_DURING_DIAGNOSE);
                }

                return;
            }
            else
            {
                DigIo::ready_out.Clear();
                diagnosePending = false;
            }
        }

        // Case 3: ignition is OFF and no diagnosis → ready must be off
        if (!diagnosePending && !ignitionOn && !DigIo::ready_out.Get())
        {
            if (DigIo::ready_safety_in.Get())
            {
                ErrorMessage::Post(ERR_READY_STUCK_ON_IGNITION_OFF);
            }
        }
    }

    void UpdateOutputs()
    {
        switch (state)
        {
        case STATE_SLEEP:
            // Deep power-down: all off
            DigIo::vcu_out.Clear();
            DigIo::condition_out.Clear();
            DigIo::ready_out.Clear();
            break;

        case STATE_STANDBY:
            // Only VCU on
            DigIo::vcu_out.Set();
            DigIo::condition_out.Clear();
            DigIo::ready_out.Clear();
            break;

        case STATE_READY:
            // All 12V systems on, HV ready
            DigIo::vcu_out.Set();
            DigIo::condition_out.Set();
            DigIo::ready_out.Set();
            break;

        case STATE_CONDITIONING:
            // VCU and conditioning on, ready off
            DigIo::vcu_out.Set();
            DigIo::condition_out.Set();
            if (!diagnosePending)
                DigIo::ready_out.Clear();
            break;

        case STATE_DRIVE:
            // All on, ready for torque
            DigIo::vcu_out.Set();
            DigIo::condition_out.Set();
            DigIo::ready_out.Set();
            break;

        case STATE_CHARGE:
            // VCU and condition on, ready off
            DigIo::vcu_out.Set();
            DigIo::condition_out.Set();
            DigIo::ready_out.Clear();
            break;

        case STATE_ERROR:
            // Fault state: all off
            DigIo::vcu_out.Set();
            DigIo::condition_out.Clear();
            DigIo::ready_out.Clear();
            break;

        case STATE_LIMP_HOME:
            // All on, degraded drive mode
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

        Param::SetInt(Param::LVDU_vcu_out, DigIo::vcu_out.Get() ? 1 : 0);
        Param::SetInt(Param::LVDU_condition_out, DigIo::condition_out.Get() ? 1 : 0);
        Param::SetInt(Param::LVDU_ready_out, DigIo::ready_out.Get() ? 1 : 0);
    }
};

#endif // LVDU_H
