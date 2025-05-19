#ifndef PDU_H
#define PDU_H

#include "params.h"
#include "hwdefs.h"
#include "digio.h"

// Fahrzeugzustände gemäß FSM
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

class PDU
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
    bool ignitionWasOn = false;

public:
    PDU() {}

    void Task10Ms()
    {
        UpdateStateByConditions();
        UpdateOutputs();
        HandleReadyDiagnosis();
        UpdateParams();
    }

private:
    void UpdateStateByConditions()
    {
        const bool ignition = DigIo::ignition_in.Get();

        // READY → DRIVE (CAN-Anforderung)
        if (state == STATE_READY && IsDriveRequestReceived())
        {
            TransitionTo(STATE_DRIVE);
        }

        // READY/DRIVE → CONDITIONING (Zündung AUS)
        if ((state == STATE_READY || state == STATE_DRIVE) && !ignition && ignitionWasOn)
        {
            TransitionTo(STATE_CONDITIONING);
        }

        // CONDITIONING → STANDBY
        if (state == STATE_CONDITIONING && CanEnterStandby())
        {
            TransitionTo(STATE_STANDBY);
        }

        // STANDBY → SLEEP
        if (state == STATE_STANDBY && CanEnterSleep())
        {
            TransitionTo(STATE_SLEEP);
        }

        ignitionWasOn = ignition;

        // Force Standby bei HV-Spannung kritisch
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

        // Force Sleep bei 12V kritisch
        if (!forceSleepActive && Is12VTooLow())
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

    void HandleReadyDiagnosis()
    {
        if (diagnosePending)
        {
            if (diagnoseTimer > 0)
            {
                diagnoseTimer--;
                DigIo::ready_out.Set(); // Relais bleibt aktiv
                if (!DigIo::ready_safety_in.Get())
                {
                    Param::SetInt(Param::lasterr, 1); // Beispielcode
                }
                return; // während Diagnose keine anderen Änderungen
            }
            else
            {
                DigIo::ready_out.Clear(); // jetzt abschalten
                diagnosePending = false;
            }
        }
    }

    void UpdateOutputs()
    {
        switch (state)
        {
        case STATE_SLEEP:
        case STATE_STANDBY:
        case STATE_ERROR:
            DigIo::vcu_out.Clear();
            DigIo::condition_out.Clear();
            DigIo::ready_out.Clear();
            break;

        case STATE_CONDITIONING:
            DigIo::vcu_out.Set();
            DigIo::condition_out.Set();
            if (!diagnosePending) DigIo::ready_out.Clear();
            break;

        case STATE_READY:
        case STATE_DRIVE:
        case STATE_LIMP_HOME:
            DigIo::vcu_out.Set();
            DigIo::condition_out.Set();
            DigIo::ready_out.Set();
            break;

        case STATE_CHARGE:
            DigIo::vcu_out.Clear();
            DigIo::condition_out.Clear();
            DigIo::ready_out.Clear();
            break;
        }
    }

    void UpdateParams()
    {
        Param::SetInt(Param::vcu_out, DigIo::vcu_out.Get() ? 1 : 0);
        Param::SetInt(Param::condition_out, DigIo::condition_out.Get() ? 1 : 0);
        Param::SetInt(Param::ready_out, DigIo::ready_out.Get() ? 1 : 0);
        Param::SetInt(Param::ready_in, DigIo::ready_in.Get() ? 1 : 0);
        Param::SetInt(Param::vehicle_state, static_cast<int>(state));
    }

    // Placeholder: Abfrage vom Inverter (z. B. CAN-Signal oder Flag)
    bool IsDriveRequestReceived()
    {
        // TODO: implementiere Abfrage
        return false;
    }

    // Bedingungen für Standby – z. B. Thermoabschaltung
    bool CanEnterStandby()
    {
        // TODO: Bedingungen einbauen
        return true;
    }

    // Bedingungen für Sleep – z. B. Timeout, keine Aktivität
    bool CanEnterSleep()
    {
        // TODO: Timer oder Wakeup-check
        return true;
    }

    // HV-Spannung prüfen (CAN/ADC)
    bool IsHVTooLow()
    {
        // TODO: echte Abfrage, z. B. return Param::GetFloat(Param::hv_voltage) < 200.0;
        return false;
    }

    // 12V überwachen (analog, digital oder ADC-Mock)
    bool Is12VTooLow()
    {
        return Read12VBatt() < 11.0f;
    }

    float Read12VBatt()
    {
        // TODO: ADC einbauen, Beispielwert:
        return 13.2f;
    }
};

#endif // PDU_H
