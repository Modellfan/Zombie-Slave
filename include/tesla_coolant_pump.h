#ifndef TESLA_COOLANT_PUMP_H
#define TESLA_COOLANT_PUMP_H

#include "params.h"
#include "hwdefs.h"
#include "digio.h"
#include "anain.h"

#define PWM_PERIOD_MS 500   // 2Hz PWM (500ms cycle)
#define PWM_RESOLUTION_MS 1 // 1ms software PWM resolution
#define MAX_RPM 4700        // Maximum RPM allowed
#define MIN_RPM 0           // Minimum RPM allowed

class TeslaCoolantPump
{
private:
    volatile uint16_t pwm_counter = 0; // PWM cycle counter (0-500ms)
    volatile uint8_t pwm_duty_cycle = 20; // Default PWM at 20% (750 RPM)

public:
    /** Default constructor */
    TeslaCoolantPump() {}

    /** Task to be executed every 100ms */
    void Task100Ms()
    {
        /*
         *  Start --> [Check coolant_pump_mode] --> (Manual?)
         *                    | Yes |                 | No |
         *           [coolant_pump_manual_value]  [coolant_pump_automatic_value]
         *                    |                          |
         *                    v                          v
         *            [Validate RPM Range] --> (RPM < 0?)
         *                    | Yes |         | No |
         *              [Set to 0 RPM]       (RPM > 4700?)
         *                      | Yes |        | No |
         *               [Set to 4700 RPM]   [Continue]
         *                      |
         *                      v
         *        [Calculate PWM Duty Cycle: PWM = (RPM + 550) / 65.8]
         *                      |
         *        [Constrain PWM within Valid Ranges]
         *                      |
         *        [Update PWM Output Value]
         *                      |
         *                      v
         *                     End
         */

        // Get operation mode (0 = manual, 1 = automatic)
        int mode = Param::GetInt(Param::coolant_pump_mode);
        int target_rpm = (mode == 0) ? Param::GetInt(Param::coolant_pump_manual_value)
                                     : Param::GetInt(Param::coolant_pump_automatic_value);

        // Constrain RPM within allowed range
        if (target_rpm < MIN_RPM)
            target_rpm = MIN_RPM;
        else if (target_rpm > MAX_RPM)
            target_rpm = MAX_RPM;

        // Convert RPM to PWM % using formula: PWM = (RPM + 550) / 65.8
        uint8_t pwm = (target_rpm + 550) / 65.8;

        // Ensure PWM is within valid ranges
        if (pwm  <= 17)
            pwm = 10; // Set to safe OFF if invalid
        else if (pwm >= 80)
            pwm = 80; // 4700 RPM

        pwm_duty_cycle = pwm; // Update global duty cycle
    }

    /** Task to be executed every 1ms for software PWM */
    void Task1Ms()
    {
        /*
         *  Start --> [Increment PWM Counter] --> (Counter >= 500ms?)
         *                          | Yes |         | No |
         *                  [Reset Counter]      [Continue]
         *                          |
         *                          v
         *        (pwm_counter < on_time?) --> [Yes] --> [Pump ON (GND)]
         *                           | No |
         *                        [Pump OFF]
         *                           |
         *                           v
         *                          End
         */

        pwm_counter++;

        if (pwm_counter >= PWM_PERIOD_MS)
        {
            pwm_counter = 0; // Reset every 500ms
        }

        // Compute ON time
        uint16_t on_time = (pwm_duty_cycle * PWM_PERIOD_MS) / 100;

        // Active low control (ON = GND, OFF = VCC)
        if (pwm_counter < on_time)
        {
            DigIo::tesla_coolant_pump_out.Clear(); // ON (grounded)
            DigIo::led_out.Set();
        }
        else
        {
            DigIo::tesla_coolant_pump_out.Set(); // OFF
            DigIo::led_out.Clear();
        }
    }
};

#endif // TESLA_COOLANT_PUMP_H
