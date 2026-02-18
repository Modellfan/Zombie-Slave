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
#define LVDU_READY_SAFETY_OFF_DELAY_STEPS 5 // 500 ms @ 100ms - ready_safety_in must be low before standby
#define VOLTAGE_DIVIDER_RATIO_12V 0.004559f // Conversion factor for dc_power_supply analog input
                                            // 12V line is scaled to the 5V ADC using an 8.2k/1.8k resistor divider
                                            // AnaIn::dc_power_supply returns millivolts, multiply by this factor to get
                                            // the actual battery voltage in volts
#define LVDU_DIAGNOSE_COOLDOWN_STEPS 2      // 200 ms cooldown after diagnosis ends
#define LVDU_HV_CONTACTOR_TIMEOUT_STEPS 100  // 10s @ 100ms - HV contactor handshake sub-state timeout

enum VehicleState
{
    STATE_INVALID = -1,
    STATE_SLEEP = 0,
    STATE_STANDBY,
    STATE_HV_CONNECTING,
    STATE_HV_DISCONNECTING,
    STATE_READY,
    STATE_CONDITIONING,
    STATE_DRIVE,
    STATE_CHARGE,
    STATE_ERROR,
    STATE_LIMP_HOME
};

enum class VehicleTriggerEvent
{
    UNKNOWN = 0,
    MANUAL_STANDBY_MODE, // manual_standby_mode == true
    AUTO_WAKE_SLEEP_TO_STANDBY, // unconditional: STATE_SLEEP -> STATE_STANDBY
    IGNITION_ON, // ignitionOn == true
    IGNITION_OFF, // ignitionOn == false
    IGNITION_ON_AND_CHARGER_NOT_PLUGGED, // ignitionOn && !chargerPlugged
    REMOTE_PRECONDITIONING_REQUESTED, // remotePreconditioningRequested == true
    CHARGER_PLUGGED_IN, // chargerPlugged == true
    CHARGER_PLUGGED_IN_AND_NOT_CHARGE_FINISHED_LATCHED, // chargerPlugged && !chargeFinishedLatched
    CHARGE_FINISHED_AND_IGNITION_OFF, // chargeFinished && !ignitionOn
    DRIVER_REQUEST_RECEIVED, // driverequestreceived == true
    CRITICAL_FAULT_DETECTED, // criticalFault == true
    DEGRADED_FAULT_DETECTED, // degradedFault == true
    THERMAL_TASK_COMPLETED_AND_READY_SAFETY_OFF_DELAY, // thermalTaskCompleted && !diagnosePending && readySafetyIn low for delay
    BMS_BALANCING_AND_BMS_INVALID_OR_LV_TOO_LOW, // bmsBalancing && (!bmsValid || is12VTooLow)
    STANDBY_TIMEOUT_EXPIRED, // standbyTimeoutCounter >= LVDU_STANDBY_TIMEOUT_STEPS
    HV_TOO_LOW_FORCE_STANDBY_TIMEOUT, // IsHVTooLow for LVDU_FORCE_DELAY_STEPS
    LV_TOO_LOW_FORCE_SLEEP_TIMEOUT, // is12VTooLow for LVDU_FORCE_DELAY_STEPS
    IGNITION_OFF_IN_ERROR, // !ignitionOn in ERROR
    HVCM_FAULT_WHILE_CONNECTING, // HVCM entered HV_FAULT while STATE_HV_CONNECTING
    HVCM_FAULT_WHILE_DISCONNECTING // HVCM entered HV_FAULT while STATE_HV_DISCONNECTING
};

// HV contactor handshake manager: cleanly separates request/feedback logic
class HvContactorManager
{
public:
    enum State
    {
        HV_DISCONNECTED = 0,
        HV_REQUESTED,
        HV_CONNECTED,
        HV_CONNECTED_STOP_CONSUMERS,
        HV_OPEN_CONTACTORS,
        HV_FAULT
    };

    HvContactorManager() : requestHV(false), state(HV_DISCONNECTED) {}

    // Update contactor handshake state (call each cycle)
    void Update()
    {
        // Read all parameters needed by the HV contactor manager.
        const bool bmsValid = Param::GetInt(Param::BMS_DataValid) != 0;
        const int contState = Param::GetInt(Param::BMS_CONT_State);
        const bool dcdcOffConfirmed = Param::GetInt(Param::dcdc_input_power_off_confirmed) != 0;
        const bool heaterOffConfirmed = Param::GetInt(Param::heater_off_confirmed) != 0;

        const bool hvClosed = bmsValid && (contState == 4); // 4 = CLOSED
        const bool hvOpen = bmsValid && (contState == 1);   // 1 = OPEN

        switch (state)
        {
        case HV_DISCONNECTED:
            if (requestHV)
            {
                SetState(HV_REQUESTED);
            }
            break;

        case HV_REQUESTED:
            if (hvClosed)
            {
                SetState(HV_CONNECTED);
            }
            else
            {
                if (stateTimeoutCounter < LVDU_HV_CONTACTOR_TIMEOUT_STEPS)
                    stateTimeoutCounter++;
                if (stateTimeoutCounter >= LVDU_HV_CONTACTOR_TIMEOUT_STEPS)
                {
                    ErrorMessage::Post(ERR_HV_CONTACTOR_TIMEOUT_CLOSING);
                    SetState(HV_FAULT);
                }
            }
            break;

        case HV_CONNECTED:
            if (!requestHV)
            {
                SetState(HV_CONNECTED_STOP_CONSUMERS);
            }
            break;

        case HV_CONNECTED_STOP_CONSUMERS:
        {
            // Transition to HV_OPEN_CONTACTORS when "stop consumers" is confirmed.
            if (dcdcOffConfirmed && heaterOffConfirmed)
            {
                SetState(HV_OPEN_CONTACTORS);
            }
            else
            {
                if (stateTimeoutCounter < LVDU_HV_CONTACTOR_TIMEOUT_STEPS)
                    stateTimeoutCounter++;
                if (stateTimeoutCounter >= LVDU_HV_CONTACTOR_TIMEOUT_STEPS)
                {
                    ErrorMessage::Post(ERR_HV_CONTACTOR_TIMEOUT_STOP_CONSUMERS);
                    SetState(HV_FAULT);
                }
            }
            break;
        }

        case HV_OPEN_CONTACTORS:
            if (hvOpen)
            {
                SetState(HV_DISCONNECTED);
            }
            else
            {
                if (stateTimeoutCounter < LVDU_HV_CONTACTOR_TIMEOUT_STEPS)
                    stateTimeoutCounter++;
                if (stateTimeoutCounter >= LVDU_HV_CONTACTOR_TIMEOUT_STEPS)
                {
                    ErrorMessage::Post(ERR_HV_CONTACTOR_TIMEOUT_OPENING);
                    SetState(HV_FAULT);
                }
            }
            break;

        case HV_FAULT:
            break;
        }

        // Publish all parameters provided by the HV contactor manager.
        const bool hvRequestToBms = (state == HV_REQUESTED || state == HV_CONNECTED || state == HV_CONNECTED_STOP_CONSUMERS);
        Param::SetInt(Param::HVCM_to_bms_hv_request, hvRequestToBms ? 1 : 0);
        Param::SetInt(Param::HVCM_state, static_cast<int>(state));
    }

    // Request HV (on/off); idempotent
    void SetHVRequest(bool enable) { requestHV = enable; }

private:
    void SetState(State newState)
    {
        if (state != newState)
        {
            state = newState;
            stateTimeoutCounter = 0;
        }
    }

    bool requestHV; // Latest request from the state machine
    State state;
    uint16_t stateTimeoutCounter = 0;
};

class LVDU
{
private:
    VehicleState state = STATE_SLEEP;
    VehicleState prevState = STATE_SLEEP;
    VehicleState prevPrevState = STATE_SLEEP;
    VehicleState queuedState = STATE_INVALID;
    VehicleTriggerEvent prevTriggerEvent = VehicleTriggerEvent::UNKNOWN;
    VehicleTriggerEvent prevPrevTriggerEvent = VehicleTriggerEvent::UNKNOWN;

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
    bool chargeFinishedLatched = false; // Remembers that the last charge cycle completed while plug stays inserted
    uint8_t readySafetyOffCounter = 0;

    // Interne Flags
    bool ignitionOn = false;
    bool readySafetyIn = false;
    float voltage12V = 13.2f; // Cached analog read, 13.2V = typical "full" battery voltage
    bool is12VTooLow = false; // Cached threshold comparison
    bool IsHVTooLow = false;
    bool chargerPlugged = false;                 // Tracks if external charger is connected
    bool remotePreconditioningRequested = false; // TODO: Add flag/CAN/schedule check via CAN
    bool thermalTaskCompleted = true;            // TODO: Implement or simulate via CAN
    bool criticalFault = false;                  // TODO: Detect via system/BMS via CAN
    bool degradedFault = false;                  // TODO: Detect via system/BMS via CAN
    bool driverequestreceived = false;           // TODO: Detect via system/BMS via CAN (fixed typo)
    bool bmsBalancing = false;                   // Tracks if BMS is currently balancing

    // HV contactor handling via manager
    HvContactorManager hvManager;

    // Store BMS state for LVDU logic
    bool bmsValid = false;

public:
    LVDU() {}

    void Task100Ms()
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
        readySafetyIn = DigIo::ready_safety_in.Get();

        // Read 12V analog input and apply voltage divider ratio
        voltage12V = AnaIn::dc_power_supply.Get() * VOLTAGE_DIVIDER_RATIO_12V;

        // Threshold evaluation (Is12VTooLow logic)
        is12VTooLow = voltage12V < Param::GetFloat(Param::LVDU_12v_low_threshold);

        // Read BMS information for HV management
        float hvVoltage = Param::GetFloat(Param::BMS_PackVoltage);
        bmsValid = Param::GetInt(Param::BMS_DataValid);
        bmsBalancing = bmsValid && Param::GetInt(Param::BMS_BalancingAnyActive);

        // Still update for possible custom behavior
        int plugStatus = Param::GetInt(Param::mlb_chr_PlugStatus);
        chargerPlugged = plugStatus > 1;        // 0=Init, 1=No Plug, 2=Plug In, 3=Plug Locked
        // Use only in case a charge plug is recognized faultly
        if (Param::GetInt(Param::Charger_Plug_Override))
        {
            chargerPlugged = false;
        }
        remotePreconditioningRequested = false; // TODO: Add flag/CAN/schedule check via CAN
        thermalTaskCompleted = true;            // TODO: Implement or simulate via CAN
        criticalFault = false;                  // TODO: Detect via system/BMS via CAN
        degradedFault = false;                  // TODO: Detect via system/BMS via CAN
        driverequestreceived = false;           // TODO: Detect via system/BMS via CAN (fixed typo)

        if (!chargerPlugged)
        {
            chargeFinishedLatched = false; // Reset latch once the plug is removed
        }

        IsHVTooLow = bmsValid && hvVoltage < Param::GetFloat(Param::LVDU_hv_low_threshold);

        // Update runtime value parameters
        Param::SetInt(Param::LVDU_ignition_in, ignitionOn ? 1 : 0);
        Param::SetInt(Param::LVDU_ready_safety_in, readySafetyIn ? 1 : 0);
        Param::SetFloat(Param::LVDU_12v_battery_voltage, voltage12V);

        hvManager.Update(); // Always keep HV manager current
    }

    void UpdateState()
    {
        // Manually force into Standby Mode. Important for Firmware flashing, because the Contactors open and close if not in standby.
        bool manualStandby = Param::GetInt(Param::manual_standby_mode);
        if (manualStandby)
        {
            forceStandbyActive = false;
            forceStandbyTimer = 0;
            forceSleepActive = false;
            forceSleepTimer = 0;
            standbyTimeoutCounter = 0;
            chargeDoneCounter = 0;

            if (state != STATE_STANDBY)
            {
                const bool hvShouldBeActive =
                    (state == STATE_CONDITIONING ||
                     state == STATE_CHARGE ||
                     state == STATE_READY ||
                     state == STATE_DRIVE ||
                     state == STATE_LIMP_HOME);

                if (hvShouldBeActive && state != STATE_HV_DISCONNECTING)
                {
                    TransitionTo(STATE_HV_DISCONNECTING, STATE_STANDBY, VehicleTriggerEvent::MANUAL_STANDBY_MODE);
                }
                else if (state != STATE_HV_DISCONNECTING)
                {
                    TransitionTo(STATE_STANDBY, VehicleTriggerEvent::MANUAL_STANDBY_MODE);
                    return;
                }
            }
            else
            {
                return;
            }
        }

        // For each state, request HV as needed, and only allow transition if HV is acknowledged closed
        switch (state)
        {
        case STATE_INVALID:
            break;

        case STATE_SLEEP:
            // Automatically transition to Standby (e.g., from wake sources)
            TransitionTo(STATE_STANDBY, VehicleTriggerEvent::AUTO_WAKE_SLEEP_TO_STANDBY);
            break;

        case STATE_HV_CONNECTING:
            hvManager.SetHVRequest(true);
            if (Param::GetInt(Param::HVCM_state) == HvContactorManager::HV_CONNECTED && queuedState != STATE_INVALID)
                TransitionTo(queuedState, prevTriggerEvent);
            else if (Param::GetInt(Param::HVCM_state) == HvContactorManager::HV_FAULT)
                TransitionTo(STATE_ERROR, VehicleTriggerEvent::HVCM_FAULT_WHILE_CONNECTING);
            break;

        case STATE_HV_DISCONNECTING:
            hvManager.SetHVRequest(false);
            if (Param::GetInt(Param::HVCM_state) == HvContactorManager::HV_DISCONNECTED && queuedState != STATE_INVALID)
                TransitionTo(queuedState, prevTriggerEvent);
            else if (Param::GetInt(Param::HVCM_state) == HvContactorManager::HV_FAULT)
                TransitionTo(STATE_ERROR, VehicleTriggerEvent::HVCM_FAULT_WHILE_DISCONNECTING);
            break;

        case STATE_STANDBY:
            if (ignitionOn && !chargerPlugged)
            {
                TransitionTo(STATE_HV_CONNECTING, STATE_READY, VehicleTriggerEvent::IGNITION_ON_AND_CHARGER_NOT_PLUGGED);
            }
            else if (remotePreconditioningRequested)
            {
                TransitionTo(STATE_HV_CONNECTING, STATE_CONDITIONING, VehicleTriggerEvent::REMOTE_PRECONDITIONING_REQUESTED);
            }
            else if (chargerPlugged)
            {
                TransitionTo(STATE_HV_CONNECTING, STATE_CHARGE, VehicleTriggerEvent::CHARGER_PLUGGED_IN);
            }
            else
            {
                if (bmsBalancing)
                {
                    standbyTimeoutCounter = 0; // Stay awake while BMS is balancing

                    // If CAN comms to the BMS fail or LV drops too low while balancing, shut down
                    if (!bmsValid || is12VTooLow)
                    {
                        TransitionTo(STATE_SLEEP, VehicleTriggerEvent::BMS_BALANCING_AND_BMS_INVALID_OR_LV_TOO_LOW);
                    }
                }
                else if (++standbyTimeoutCounter >= LVDU_STANDBY_TIMEOUT_STEPS)
                {
                    TransitionTo(STATE_SLEEP, VehicleTriggerEvent::STANDBY_TIMEOUT_EXPIRED);
                }
            }
            break;

        case STATE_READY:
            if (!ignitionOn)
                TransitionTo(STATE_CONDITIONING, VehicleTriggerEvent::IGNITION_OFF);
            else if (driverequestreceived)
                TransitionTo(STATE_DRIVE, VehicleTriggerEvent::DRIVER_REQUEST_RECEIVED);
            else if (chargerPlugged)
                TransitionTo(STATE_CHARGE, VehicleTriggerEvent::CHARGER_PLUGGED_IN);
            else if (criticalFault)
                TransitionTo(STATE_HV_DISCONNECTING, STATE_ERROR, VehicleTriggerEvent::CRITICAL_FAULT_DETECTED);
            break;

        case STATE_CONDITIONING:
            if (criticalFault)
            {
                TransitionTo(STATE_HV_DISCONNECTING, STATE_ERROR, VehicleTriggerEvent::CRITICAL_FAULT_DETECTED);
                break;
            }
            if (ignitionOn)
            {
                TransitionTo(STATE_READY, VehicleTriggerEvent::IGNITION_ON);
                break;
            }
            if (chargerPlugged && !chargeFinishedLatched)
            {
                TransitionTo(STATE_CHARGE, VehicleTriggerEvent::CHARGER_PLUGGED_IN_AND_NOT_CHARGE_FINISHED_LATCHED);
                break;
            }
            if (thermalTaskCompleted && !diagnosePending)
            {
                // Ensure ready_safety_in is low for a defined period before standby.
                // This helps ensure the inverter is off; otherwise it can hold the
                // precharge relay and damage the precharge resistor while heater/DCDC
                // still draw current.
                if (!readySafetyIn)
                {
                    if (readySafetyOffCounter < LVDU_READY_SAFETY_OFF_DELAY_STEPS)
                        readySafetyOffCounter++;

                    if (readySafetyOffCounter >= LVDU_READY_SAFETY_OFF_DELAY_STEPS)
                    {
                        TransitionTo(STATE_HV_DISCONNECTING, STATE_STANDBY, VehicleTriggerEvent::THERMAL_TASK_COMPLETED_AND_READY_SAFETY_OFF_DELAY);
                    }
                }
                else
                {
                    readySafetyOffCounter = 0;
                }
            }
            else
            {
                readySafetyOffCounter = 0;
            }
            break;

        case STATE_DRIVE:
            if (!ignitionOn)
                TransitionTo(STATE_CONDITIONING, VehicleTriggerEvent::IGNITION_OFF);
            else if (chargerPlugged)
                TransitionTo(STATE_CHARGE, VehicleTriggerEvent::CHARGER_PLUGGED_IN);
            else if (degradedFault)
                TransitionTo(STATE_LIMP_HOME, VehicleTriggerEvent::DEGRADED_FAULT_DETECTED);
            break;

        case STATE_CHARGE:
        {
            // Charge done definition on no current. That can also be the case, when the plug is removed
            float doneCurrent = Param::GetFloat(Param::charge_done_current);
            float actualCurrent = Param::GetFloat(Param::BMS_ActualCurrent);
            int delaySteps = Param::GetInt(Param::charge_done_delay) * 10;

            bool chargeFinished = false;
            if (delaySteps <= 0)
            {
                chargeFinished = true;
                chargeDoneCounter = 0;
            }
            else if (actualCurrent < doneCurrent)
            {
                if (chargeDoneCounter < delaySteps)
                    ++chargeDoneCounter;

                if (chargeDoneCounter >= delaySteps)
                    chargeFinished = true;
            }
            else
            {
                chargeDoneCounter = 0;
            }

            // Directly jump to ready when unplugged and ignition is on
            if ((!chargerPlugged) && (ignitionOn))
            {
                TransitionTo(STATE_READY, VehicleTriggerEvent::IGNITION_ON_AND_CHARGER_NOT_PLUGGED);
                chargeDoneCounter = 0;
            }

            if (criticalFault)
            {
                TransitionTo(STATE_HV_DISCONNECTING, STATE_ERROR, VehicleTriggerEvent::CRITICAL_FAULT_DETECTED);
                chargeDoneCounter = 0;
                break;
            }

            // Only jump to conditioning, when finished and no ignition on. When ever a plug is inserted and ignition is on, it should not transition to ready. This will avoid later to drive away with a plug inserted.
            if ((chargeFinished) && (!ignitionOn))
            {
                chargeFinishedLatched = true;
                TransitionTo(STATE_CONDITIONING, VehicleTriggerEvent::CHARGE_FINISHED_AND_IGNITION_OFF);
                chargeDoneCounter = 0;
            }
        }
        break;

    case STATE_ERROR:
        if (!ignitionOn)
            TransitionTo(STATE_HV_DISCONNECTING, STATE_SLEEP, VehicleTriggerEvent::IGNITION_OFF_IN_ERROR);
        break;

    case STATE_LIMP_HOME:
        if (!ignitionOn)
            TransitionTo(STATE_CONDITIONING, VehicleTriggerEvent::IGNITION_OFF);
        else if (chargerPlugged)
            TransitionTo(STATE_CHARGE, VehicleTriggerEvent::CHARGER_PLUGGED_IN);
        break;
    }

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
            TransitionTo(STATE_HV_DISCONNECTING, STATE_STANDBY, VehicleTriggerEvent::HV_TOO_LOW_FORCE_STANDBY_TIMEOUT);
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
            TransitionTo(STATE_SLEEP, VehicleTriggerEvent::LV_TOO_LOW_FORCE_SLEEP_TIMEOUT);
        }
    }
}

void TransitionTo(VehicleState newState, VehicleState queuedStateNext, VehicleTriggerEvent triggerEvent)
{
    prevPrevState = prevState;
    prevState = state;
    state = newState;
    prevPrevTriggerEvent = prevTriggerEvent;
    prevTriggerEvent = triggerEvent;
    if (newState == STATE_HV_CONNECTING || newState == STATE_HV_DISCONNECTING)
        queuedState = queuedStateNext;
    else
        queuedState = STATE_INVALID;
    // Reset standbyTimeoutCounter when entering STANDBY
    if (newState == STATE_STANDBY)
    {
        standbyTimeoutCounter = 0;
    }
    if (newState != STATE_CONDITIONING)
    {
        readySafetyOffCounter = 0;
    }

    // READY → CONDITIONING: Start diagnosis
    if (prevState == STATE_READY && newState == STATE_CONDITIONING)
    {
        diagnosePending = true;
        diagnoseTimer = LVDU_DIAGNOSE_DELAY_STEPS;
    }

    if (newState == STATE_CHARGE)
    {
        chargeDoneCounter = 0;
        chargeFinishedLatched = false;
    }
}

void TransitionTo(VehicleState newState, VehicleTriggerEvent triggerEvent)
{
    TransitionTo(newState, STATE_INVALID, triggerEvent);
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
    case STATE_INVALID:
        break;

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

    case STATE_HV_CONNECTING:
        // Handshake phase: keep control power on, but do not assert condition/ready yet.
        DigIo::vcu_out.Set();
        DigIo::condition_out.Set();
        DigIo::ready_out.Clear(); //This hopefully removes all interferences with SDU activating the precharge
        break;

    case STATE_HV_DISCONNECTING:
        // Handshake phase: keep control power on until HV is safely opened.
        DigIo::vcu_out.Set();
        DigIo::condition_out.Set();
        DigIo::ready_out.Clear(); //This hopefully removes all interferences with SDU activating the precharge
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

void UpdateParams()
{
    Param::SetInt(Param::LVDU_vehicle_state, static_cast<int>(state));
    Param::SetInt(Param::LVDU_last_vehicle_state, static_cast<int>(prevState));
    Param::SetInt(Param::LVDU_prev_prev_vehicle_state, static_cast<int>(prevPrevState));
    Param::SetInt(Param::LVDU_queued_vehicle_state, static_cast<int>(queuedState));
    Param::SetInt(Param::LVDU_prev_trigger_event, static_cast<int>(prevTriggerEvent));
    Param::SetInt(Param::LVDU_prev_prev_trigger_event, static_cast<int>(prevPrevTriggerEvent));

    Param::SetInt(Param::LVDU_diagnose_pending, diagnosePending ? 1 : 0);

    Param::SetInt(Param::LVDU_vcu_out, DigIo::vcu_out.Get() ? 1 : 0);
    Param::SetInt(Param::LVDU_condition_out, DigIo::condition_out.Get() ? 1 : 0);
    Param::SetInt(Param::LVDU_ready_out, DigIo::ready_out.Get() ? 1 : 0);

    // Comfort functions only allowed when HV is connected.
    const int hvState = Param::GetInt(Param::HVCM_state);
    Param::SetInt(Param::hv_comfort_functions_allowed, hvState == HvContactorManager::HV_CONNECTED ? 1 : 0);
}
}
;

#endif // LVDU_H
