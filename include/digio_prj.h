#ifndef PinMode_PRJ_H_INCLUDED
#define PinMode_PRJ_H_INCLUDED

#include "hwdefs.h"

/* Here you specify generic IO pins, i.e. digital input or outputs.
 * Inputs can be floating (INPUT_FLT), have a 30k pull-up (INPUT_PU)
 * or pull-down (INPUT_PD) or be an output (OUTPUT)
 * !!Pull-Up does not work on Zombie because externally pulled down!! */

#define DIG_IO_LIST                                                             \
    DIG_IO_ENTRY(led_out, GPIOE, GPIO2, PinMode::OUTPUT)                        \
    DIG_IO_ENTRY(tesla_coolant_valve_1_out, GPIOD, GPIO13, PinMode::OUTPUT)     \
    DIG_IO_ENTRY(tesla_coolant_valve_2_out, GPIOD, GPIO14, PinMode::OUTPUT)     \
    DIG_IO_ENTRY(tesla_coolant_pump_out, GPIOD, GPIO15, PinMode::OUTPUT)        \
    DIG_IO_ENTRY(ignition_in, GPIOD, GPIO6, PinMode::INPUT_PD)                  \
    DIG_IO_ENTRY(ready_safety_in, GPIOA, GPIO15, PinMode::INPUT_PD)             \
    DIG_IO_ENTRY(ready_out, GPIOA, GPIO8, PinMode::OUTPUT)                      \
    DIG_IO_ENTRY(condition_out, GPIOC, GPIO7, PinMode::OUTPUT)                  \
    DIG_IO_ENTRY(vcu_out, GPIOD, GPIO12, PinMode::OUTPUT)                       \
    DIG_IO_ENTRY(vacuum_pump_out, GPIOC, GPIO9, PinMode::OUTPUT)                \
    DIG_IO_ENTRY(vacuum_sensor_in, GPIOD, GPIO4, PinMode::INPUT_PD)             \
    DIG_IO_ENTRY(heater_contactor_out, GPIOC, GPIO6, PinMode::OUTPUT)           \
    DIG_IO_ENTRY(heater_contactor_feedback_in, GPIOD, GPIO5, PinMode::INPUT_PU) \
    DIG_IO_ENTRY(servo_pump_out, GPIOB, GPIO0, PinMode::OUTPUT)                 \
    DIG_IO_ENTRY(eps_quick_spoolup_out, GPIOA, GPIO7, PinMode::OUTPUT)\
    DIG_IO_ENTRY(eps_ignition_on_out, GPIOA, GPIO6, PinMode::OUTPUT)\
    DIG_IO_ENTRY(heater_thermal_switch_in, GPIOD, GPIO7, PinMode::INPUT_PD)
#endif // PinMode_PRJ_H_INCLUDED
