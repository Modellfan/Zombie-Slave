#ifndef LVDU_H
#define LVDU_H

#include "params.h"
#include "hwdefs.h"
#include "digio.h"
#include "errormessage.h"
#include "anain.h"

// Configuration macros
#define LVDU_DIAGNOSE_DELAY_STEPS 40        // 4000 ms - How long hold the relay after ignition off to check, if the relay is working
#define LVDU_READY_DELAY_STEPS 20           // 2000 ms - How long is the hysteresis, after that the ready_in should follow the ignition
#define LVDU_FORCE_DELAY_STEPS 200          // 20s - How long after force should down because of low HV or LV
#define LVDU_STANDBY_TIMEOUT_STEPS 100      // 10s @ 100ms - How long in standby to shut down
#define VOLTAGE_DIVIDER_RATIO_12V 0.004559f // Conversion factor for dc_power_supply analog input
                                            // 12V line is scaled to the 5V ADC using an 8.2k/1.8k resistor divider
                                            // AnaIn::dc_power_supply returns millivolts, multiply by this factor to get
                                            // the actual battery voltage in volts
#define LVDU_DIAGNOSE_COOLDOWN_STEPS 2 // 200 ms cooldown after diagnosis ends

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
    uint16_t diagnoseCooldownTimer = 0;

    // Force Standby/Sleep
    bool forceStandbyActive = false;
    bool forceSleepActive = false;
    uint16_t forceStandbyTimer = 0;
    uint16_t forceSleepTimer = 0;
    uint16_t standbyTimeoutCounter = 0;
    uint16_t chargeDoneCounter = 0;

    // Interne Flags
    bool ignitionOn = false;
    float voltage12V = 13.2f; // Cached analog read, 13.2V = typical "full" battery voltage
    bool is12VTooLow = false; // Cached threshold comparison
    bool IsHVTooLow = false;
    bool chargerPlugged = false;                 // TODO: Add real charger detection via CAN
    bool remotePreconditioningRequested = false; // TODO: Add flag/CAN/schedule check via CAN
    bool thermalTaskCompleted = true;            // TODO: Implement or simulate via CAN
    bool criticalFault = false;                  // TODO: Detect via system/BMS via CAN
    bool degradedFault = false;                  // TODO: Detect via system/BMS via CAN
    bool driverequestreceived = false;           // TODO: Detect via system/BMS via CAN (fixed typo)
    bool manualChargePrev = false;               // Tracks previous manual_charge_mode value

    // HV transition handling
    bool hvRequestPending = false;                 // Waiting for BMS contactor close
    VehicleState hvRequestedState = STATE_STANDBY; // Target state once HV is on
    bool hvContactorsClosed = false;               // BMS feedback

public:
    LVDU() {}

    void Task100Ms()
    {
        UpdateInputs();
        UpdateState();
        HandleReadyDiagnosis();
        UpdateOutputs();
        HandleLow12V();
        UpdateParams();
    }

private:
    void UpdateInputs()
    {
        // Read digital inputs
        ignitionOn = DigIo::ignition_in.Get();
        bool readySafety = DigIo::ready_safety_in.Get();

        // Read 12V analog input and apply voltage divider ratio
        voltage12V = AnaIn::dc_power_supply.Get() * VOLTAGE_DIVIDER_RATIO_12V;

        // Threshold evaluation (Is12VTooLow logic)
        is12VTooLow = voltage12V < Param::GetFloat(Param::LVDU_12v_low_threshold);

        // Read BMS information for HV management
        float hvVoltage = Param::GetFloat(Param::BMS_PackVoltage);
        int bmsValid = Param::GetInt(Param::BMS_DataValid);
        int contState = Param::GetInt(Param::BMS_CONT_State);

        hvContactorsClosed = bmsValid && contState == 4; // 4=CLOSED
        IsHVTooLow = bmsValid && hvVoltage < Param::GetFloat(Param::LVDU_hv_low_threshold);

        // Update runtime value parameters
        Param::SetInt(Param::LVDU_ignition_in, ignitionOn ? 1 : 0);
        Param::SetInt(Param::LVDU_ready_safety_in, readySafety ? 1 : 0);
        Param::SetFloat(Param::LVDU_12v_battery_voltage, voltage12V);
    }

    void UpdateState()
    {
        // Manual override: detect rising/falling edge of manual_charge_mode
        bool manualCharge = Param::GetInt(Param::manual_charge_mode);
        if (manualCharge && !manualChargePrev && state != STATE_CHARGE)
        {
            // Rising edge → force Charge state
            RequestHVState(STATE_CHARGE);
        }
        else if (!manualCharge && manualChargePrev && state == STATE_CHARGE)
        {
            // Falling edge → exit Charge as if charger unplugged
            TransitionTo(STATE_CONDITIONING);
        }
        manualChargePrev = manualCharge;

        // Wait for HV contactor acknowledgment before entering HV states
        if (hvRequestPending)
        {
            bool requestValid = true;
            switch (hvRequestedState)
            {
            case STATE_READY:
                requestValid = ignitionOn;
                break;
            case STATE_CONDITIONING:
                requestValid = remotePreconditioningRequested;
                break;
            case STATE_CHARGE:
                requestValid = chargerPlugged || manualCharge;
                break;
            default:
                break;
            }

            if (!requestValid)
            {
                hvRequestPending = false; // cancel request if conditions change
            }
            else if (hvContactorsClosed)
            {
                hvRequestPending = false;
                TransitionTo(hvRequestedState);
                return;
            }
            else
            {
                return; // hold current state until contactors closed
            }
        }

        switch (state)
        {
        case STATE_SLEEP:
            // Automatically transition to Standby (e.g., from wake sources)
            TransitionTo(STATE_STANDBY);
            break;

        case STATE_STANDBY:
            if (ignitionOn)
            {
                RequestHVState(STATE_READY);
            }
            else if (remotePreconditioningRequested)
            {
                RequestHVState(STATE_CONDITIONING);
            }
            else if (chargerPlugged)
            {
                RequestHVState(STATE_CHARGE);
            }
            else
            {
                if (++standbyTimeoutCounter >= LVDU_STANDBY_TIMEOUT_STEPS)
                {
                    TransitionTo(STATE_SLEEP);
                }
            }
            break;

        case STATE_READY:
            if (!ignitionOn)
                TransitionTo(STATE_CONDITIONING);
            else if (driverequestreceived)
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
            else if (thermalTaskCompleted && !diagnosePending)
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
        {
            if (!chargerPlugged)
            {
                TransitionTo(STATE_CONDITIONING);
                chargeDoneCounter = 0;
                break;
            }
            if (criticalFault)
            {
                TransitionTo(STATE_ERROR);
                chargeDoneCounter = 0;
                break;
            }

            float doneCurrent = Param::GetFloat(Param::charge_done_current);
            float actualCurrent = Param::GetFloat(Param::BMS_ActualCurrent);
            if (actualCurrent < 0)
                actualCurrent = -actualCurrent;
            int delaySteps = Param::GetInt(Param::charge_done_delay) * 10;

            if (actualCurrent < doneCurrent)
            {
                if (chargeDoneCounter < delaySteps)
                    ++chargeDoneCounter;
            }
            else
            {
                chargeDoneCounter = 0;
            }

            if (chargeDoneCounter >= delaySteps && delaySteps > 0)
            {
                TransitionTo(STATE_CONDITIONING);
                chargeDoneCounter = 0;
            }
        }
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

        if (state != STATE_CHARGE)
            chargeDoneCounter = 0;

        // --- Safe decrement for force timers to prevent underflow

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

        if (forceStandbyActive && forceStandbyTimer > 0)
        {
            --forceStandbyTimer;
            if (forceStandbyTimer == 0)
            {
                forceStandbyActive = false;
                TransitionTo(STATE_STANDBY);
            }
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

        if (forceSleepActive && forceSleepTimer > 0)
        {
            --forceSleepTimer;
            if (forceSleepTimer == 0)
            {
                forceSleepActive = false;
                TransitionTo(STATE_SLEEP);
            }
        }
    }

    void TransitionTo(VehicleState newState)
    {
        lastState = state;
        state = newState;

        if (!(newState == STATE_READY || newState == STATE_CONDITIONING ||
              newState == STATE_DRIVE || newState == STATE_CHARGE ||
              newState == STATE_LIMP_HOME))
        {
            hvRequestPending = false;
        }

        // Reset standbyTimeoutCounter when entering STANDBY
        if (newState == STATE_STANDBY)
        {
            standbyTimeoutCounter = 0;
        }

        // READY → CONDITIONING: Start diagnosis
        if (lastState == STATE_READY && newState == STATE_CONDITIONING)
        {
            diagnosePending = true;
            diagnoseTimer = LVDU_DIAGNOSE_DELAY_STEPS;
        }
    }

    void RequestHVState(VehicleState target)
    {
        hvRequestPending = true;
        hvRequestedState = target;
    }

    void HandleReadyDiagnosis()
    {
        // Reset readySetDelayTimer if ready_safety_in is set during the window

        // Case 1: ignition ON → ready_safety_in must follow within 2000ms (LVDU_READY_DELAY_STEPS)
        if (ignitionOn)
        {
            if (DigIo::ready_safety_in.Get())
            {
                readySetDelayTimer = 0; // Success, reset early
            }
            else if (readySetDelayTimer < LVDU_READY_DELAY_STEPS)
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

                // Error: ready_safety_in dropped during diagnose
                if (!DigIo::ready_safety_in.Get())
                {
                    ErrorMessage::Post(ERR_READY_DROPPED_DURING_DIAGNOSE);
                }

                return;
            }
            else
            {
                diagnosePending = false;
                diagnoseCooldownTimer = LVDU_DIAGNOSE_COOLDOWN_STEPS;
            }
        }

        // Case 3: ignition is OFF and no diagnosis → ready must be off
        if (!diagnosePending && !ignitionOn)
        {
            if (diagnoseCooldownTimer > 0)
            {
                diagnoseCooldownTimer--;
            }
            else if (DigIo::ready_safety_in.Get())
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
            if (diagnosePending)
                DigIo::ready_out.Set();
            else
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

    void HandleLow12V()
    {
        if (state != STATE_READY && state != STATE_DRIVE && state != STATE_LIMP_HOME)
        {
            float threshold = Param::GetFloat(Param::LVDU_12v_low_threshold);
            if (voltage12V < threshold)
            {
                DigIo::vcu_out.Clear();
            }
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

        // For now comfort functions are always permitted
        Param::SetInt(Param::hv_comfort_functions_allowed, 1);

        int connectHV = 0;
        if (hvRequestPending || state == STATE_READY || state == STATE_CONDITIONING ||
            state == STATE_DRIVE || state == STATE_CHARGE ||
            state == STATE_LIMP_HOME)
        {
            connectHV = 1;
        }
        Param::SetInt(Param::LVDU_connectHVcommand, connectHV);
    }
};

#endif // LVDU_H
