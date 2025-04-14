#ifndef TESLA_VALVE_H
#define TESLA_VALVE_H

#include "params.h"
#include "anain.h"
#include "hwdefs.h"
#include "digio.h"

#define VOLTAGE_DIVIDER_RATIO 0.0059f // 4.9k to 1k voltage divider
#define VALVE_90_DEG_VOLTAGE 11.71f   // Example threshold voltage
#define VALVE_180_DEG_VOLTAGE 3.06f   // Example threshold voltage
#define VALVE_TOLERANCE 0.25f         // Tolerance for determining valve state

class TeslaValve
{
public:
    /** Default constructor */
    TeslaValve() {}

    /** Write parameter to the digital pin */
    void WriteValveState()
    {
        int valveState = Param::GetInt(Param::valve_out);
        if (valveState == 1)
        {
            DigIo::tesla_coolant_valve_1_out.Set();
        }
        else
        {
            DigIo::tesla_coolant_valve_1_out.Clear();
        }
    }

    /** Read the analog pin, process value and update the parameter */
    void ReadValveState()
    {
        float rawVoltage = AnaIn::tesla_coolant_valve_1_in.Get() * VOLTAGE_DIVIDER_RATIO;
        Param::SetFloat(Param::valve_in_raw, rawVoltage);

        if (rawVoltage >= (VALVE_90_DEG_VOLTAGE - VALVE_TOLERANCE) && rawVoltage <= (VALVE_90_DEG_VOLTAGE + VALVE_TOLERANCE))
        {
            Param::SetInt(Param::valve_in, 1); // 90°
        }
        else if (rawVoltage >= (VALVE_180_DEG_VOLTAGE - VALVE_TOLERANCE) && rawVoltage <= (VALVE_180_DEG_VOLTAGE + VALVE_TOLERANCE))
        {
            Param::SetInt(Param::valve_in, 0); // 180°
        }
        else
        {
            Param::SetInt(Param::valve_in, 2); // In Transition
        }
    }

    /** Control logic for auto mode */
    void ControlAutoMode()
    {
        int autoTarget = Param::GetInt(Param::valve_auto_target);
        if (autoTarget == 1)
        {
            DigIo::tesla_coolant_valve_1_out.Set(); // Set to 90°
        }
        else
        {
            DigIo::tesla_coolant_valve_1_out.Clear(); // Set to 180°
        }
    }

    /** Task to be executed every 100ms */
    void Task100Ms()
    {
        ReadValveState();
        if (Param::GetInt(Param::valve_out) == 2)
        {
            ControlAutoMode();
        }
        else
        {
            WriteValveState();
        }
    }
};

#endif // TESLA_VALVE_H
