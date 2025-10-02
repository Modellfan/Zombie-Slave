/*
 * This file is part of the stm32-template project.
 *
 * Copyright (C) 2020 Johannes Huebner <dev@johanneshuebner.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This file contains all parameters used in your project
 * See main.cpp on how to access them.
 * If a parameters unit is of format "0=Choice, 1=AnotherChoice" etc.
 * It will be displayed as a dropdown in the web interface
 * If it is a spot value, the decimal is translated to the name, i.e. 0 becomes "Choice"
 * If the enum values are powers of two, they will be displayed as flags, example
 * "0=None, 1=Flag1, 2=Flag2, 4=Flag3, 8=Flag4" and the value is 5.
 * It means that Flag1 and Flag3 are active -> Display "Flag1 | Flag3"
 *
 * Every parameter/value has a unique ID that must never change. This is used when loading parameters
 * from flash, so even across firmware versions saved parameters in flash can always be mapped
 * back to our list here. If a new value is added, it will receive its default value
 * because it will not be found in flash.
 * The unique ID is also used in the CAN module, to be able to recover the CAN map
 * no matter which firmware version saved it to flash.
 * Make sure to keep track of your ids and avoid duplicates. Also don't re-assign
 * IDs from deleted parameters because you will end up loading some random value
 * into your new parameter!
 * IDs are 16 bit, so 65535 is the maximum
 */

// Define a version string of your firmware here
#define VER 2.01.R
#define VERSTR STRINGIFY(4 = VER)

/* Entries must be ordered as follows:
   1. Saveable parameters (id != 0)
   2. Temporary parameters (id = 0)
   3. Display values
 */
// Next param id (increase when adding new parameter!): 166
// Next value Id: 2333
/*              category     name         unit       min     max     default id */
#define PARAM_LIST                                                                        \
   PARAM_ENTRY(CAT_COMM, canspeed, CANSPEEDS, 0, 4, 2, 1)                                 \
   PARAM_ENTRY(CAT_COMM, canperiod, CANPERIODS, 0, 1, 0, 2)                               \
   PARAM_ENTRY(CAT_VALVE, valve_out_1, VALVE, 0, 2, 0, 100)                               \
   PARAM_ENTRY(CAT_TESLA_COOLANT_PUMP, coolant_pump_mode, AUTO_MANUAL, 0, 1, 0, 101)      \
   PARAM_ENTRY(CAT_TESLA_COOLANT_PUMP, coolant_pump_manual_value, "RPM", 0, 4700, 0, 102) \
   PARAM_ENTRY(CAT_EPS, eps_spoolup_delay, "ms", 0, 5000, 500, 106)                       \
   PARAM_ENTRY(CAT_VACUUM_PUMP, vacuum_hysteresis, "ms", 0, 60000, 500, 104)              \
   PARAM_ENTRY(CAT_VACUUM_PUMP, vacuum_warning_delay, "ms", 0, 60000, 2000, 105)          \
   PARAM_ENTRY(CAT_SETUP, dcdc_can, CAN_DEV, 0, 1, 1, 107)                                \
   PARAM_ENTRY(CAT_SETUP, chargemodes, CHGMODS, 0, 8, 0, 37)                              \
   PARAM_ENTRY(CAT_TESLA_DCDC, dcdc_voltage_setpoint, "V", 9, 16, 13.5, 108)              \
   PARAM_ENTRY(CAT_LVDU, LVDU_12v_low_threshold, "V", 8.0, 13.5, 11.0, 116)               \
   PARAM_ENTRY(CAT_LVDU, LVDU_hv_low_threshold, "V", 100.0, 800.0, 200.0, 117)            \
   PARAM_ENTRY(CAT_LVDU, manual_charge_mode, YESNO, 0, 1, 0, 118)                         \
   PARAM_ENTRY(CAT_LVDU, manual_standby_mode, YESNO, 0, 1, 0, 121)                        \
                                                                                          \
   PARAM_ENTRY(CAT_LVDU, charge_done_current, "A", 0, 10, 0.5, 119)                       \
                                                                                          \
   PARAM_ENTRY(CAT_LVDU, charge_done_delay, "s", 0, 600, 30, 120)                         \
#ifdef MLB_CHARGER_STANDALONE                                                             \
   PARAM_ENTRY(CAT_MLB_SIM, mlb_chr_sim_SOC, "%", 0, 100, 50, 153)                        \
   PARAM_ENTRY(CAT_MLB_SIM, mlb_chr_sim_SOC_Target, "%", 0, 100, 100, 154)                \
   PARAM_ENTRY(CAT_MLB_SIM, mlb_chr_sim_BMSMinVolt, "V", 0, 1000, 0, 155)                 \
   PARAM_ENTRY(CAT_MLB_SIM, mlb_chr_sim_IDCSetpnt, "A", 0, 200, 10, 156)                  \
   PARAM_ENTRY(CAT_MLB_SIM, mlb_chr_sim_HVDCSetpnt, "V", 0, 1000, 400, 157)               \
   PARAM_ENTRY(CAT_MLB_SIM, mlb_chr_sim_BMSBattCellSum, "V", 0, 1000, 350, 158)           \
   PARAM_ENTRY(CAT_MLB_SIM, mlb_chr_sim_BMSMaxVolt, "V", 0, 1000, 400, 159)               \
   PARAM_ENTRY(CAT_MLB_SIM, mlb_chr_sim_BMS_Cell_H_Temp, "°C", -40, 100, 25, 160)         \
   PARAM_ENTRY(CAT_MLB_SIM, mlb_chr_sim_BMS_Cell_L_Temp, "°C", -40, 100, 20, 161)         \
   PARAM_ENTRY(CAT_MLB_SIM, mlb_chr_sim_BMS_Cell_H_mV, "mV", 0, 5000, 4200, 162)          \
   PARAM_ENTRY(CAT_MLB_SIM, mlb_chr_sim_BMS_Cell_L_mV, "mV", 0, 5000, 3000, 163)          \
   PARAM_ENTRY(CAT_MLB_SIM, mlb_chr_sim_Activation_Crg, "dig", 0, 1, 0, 164)              \
   PARAM_ENTRY(CAT_MLB_SIM, mlb_chr_sim_Lock, "dig", 0, 1, 0, 165)                        \
#endif                                                                                     \
                                                                                          \
   VALUE_ENTRY(version, VERSTR, 2001)                                                     \
   VALUE_ENTRY(lasterr, errorListString, 2002)                                            \
   VALUE_ENTRY(cpuload, "%", 2004)                                                        \
   VALUE_ENTRY(valve_in_raw, "V", 2100)                                                   \
   VALUE_ENTRY(valve_in, VALVE_STATE, 2101)                                               \
   VALUE_ENTRY(valve_auto_target, VALVE_TARGET, 2102)                                     \
   VALUE_ENTRY(coolant_pump_automatic_value, "RPM", 2103)                                 \
   VALUE_ENTRY(coolant_pump_status, "On/Off", 2104)                                       \
   VALUE_ENTRY(coolant_pump_fault, "Error", 2105)                                         \
                                                                                          \
   VALUE_ENTRY(vacuum_pump_out, "On/Off", 2109)                                           \
   VALUE_ENTRY(vacuum_sensor, VACUUM_STATE, 2110)                                         \
   VALUE_ENTRY(vacuum_pump_insufficient, YESNO, 2111)                                     \
                                                                                          \
   VALUE_ENTRY(dcdc_coolant_temp, "°C", 2112)                                             \
   VALUE_ENTRY(dcdc_input_power, "W", 2113)                                               \
   VALUE_ENTRY(dcdc_output_voltage, "V", 2114)                                            \
   VALUE_ENTRY(dcdc_output_current, "A", 2115)                                            \
   VALUE_ENTRY(mlb_chr_DC_Max_ChargePower, "W", 2300)                                     \
   VALUE_ENTRY(mlb_chr_DC_Max_ChargeVoltage, "V", 2301)                                   \
   VALUE_ENTRY(mlb_chr_DC_Actual_Current, "A", 2302)                                      \
   VALUE_ENTRY(mlb_chr_DC_Max_ChargeCurrent, "A", 2303)                                   \
   VALUE_ENTRY(mlb_chr_DC_Min_ChargeVoltage, "V", 2304)                                   \
   VALUE_ENTRY(mlb_chr_DC_Min_ChargeCurrent, "A", 2305)                                   \
   VALUE_ENTRY(mlb_chr_Status_Grid, "dig", 2306)                                         \
   VALUE_ENTRY(mlb_chr_ChargeManagerMode, HVLM_OPMODE, 2307)                               \
   VALUE_ENTRY(mlb_chr_ChargerRequestingHV, HVLM_ACTREQ, 2308)                             \
   VALUE_ENTRY(mlb_chr_ChargerErrorStatus, HVLM_ERRORSTATUS, 2309)                         \
   VALUE_ENTRY(mlb_chr_PlugStatus, HVLM_PLUGSTATUS, 2310)                                  \
   VALUE_ENTRY(mlb_chr_LoadRequest, HVLM_LOADREQ, 2311)                                    \
   VALUE_ENTRY(mlb_chr_ChargerState, HVLM_CHARGERMODE, 2312)                               \
   VALUE_ENTRY(mlb_chr_Charger_AC_Volt_RMS, "V", 2313)                                   \
   VALUE_ENTRY(mlb_chr_Charger_VoltageOut_HV, "V", 2314)                                 \
   VALUE_ENTRY(mlb_chr_Charger_CurrentOut_HV, "A", 2315)                                 \
   VALUE_ENTRY(mlb_chr_Charger_Temperature, "°C", 2316)                                  \
   VALUE_ENTRY(mlb_chr_ChargerSystemState, HVLM_CHARGESYSSTATE, 2317)                      \
   VALUE_ENTRY(mlb_chr_Status_LED, "dig", 2318)                                          \
   VALUE_ENTRY(mlb_chr_MaxCurrent_AC, "A", 2319)                                          \
   VALUE_ENTRY(mlb_chr_LockRequest, HVLM_CONNECTORLOCK, 2320)                              \
   VALUE_ENTRY(mlb_chr_Charger_Ready, HVLM_CHGREADY, 2321)                                 \
   VALUE_ENTRY(mlb_chr_ChargerTemp_Reduction, "dig", 2322)                                \
   VALUE_ENTRY(mlb_chr_ChargerCurrent_Reduction, "dig", 2323)                             \
   VALUE_ENTRY(mlb_chr_SocketTemp_Reduction, "dig", 2324)                                 \
   VALUE_ENTRY(mlb_chr_MaxChargerOutput, "W", 2325)                                       \
   VALUE_ENTRY(mlb_chr_CableCurrentLimit, "A", 2326)                                      \
   VALUE_ENTRY(mlb_chr_ControlPilotStatus, "dig", 2327)                                   \
   VALUE_ENTRY(mlb_chr_LockState, "dig", 2328)                                            \
   VALUE_ENTRY(mlb_chr_ChargerWarning, "dig", 2329)                                       \
   VALUE_ENTRY(mlb_chr_ChargerFault, "dig", 2330)                                         \
   VALUE_ENTRY(mlb_chr_OutputVolts, "V", 2331)                                            \
   VALUE_ENTRY(mlb_chr_ActivationState, "dig", 2332)                                      \
   VALUE_ENTRY(dcdc_fault_any, YESNO, 2116)                                               \
                                                                                          \
   PARAM_ENTRY(CAT_BMS, BMS_CAN, CAN_DEV, 0, 1, 1, 110)                                   \
   VALUE_ENTRY(BMS_Vmin, "V", 2201)                                                       \
   VALUE_ENTRY(BMS_Vmax, "V", 2202)                                                       \
   VALUE_ENTRY(BMS_Tmin, "°C", 2203)                                                      \
   VALUE_ENTRY(BMS_Tmax, "°C", 2204)                                                      \
   VALUE_ENTRY(BMS_SOC, "%", 2205)                                                        \
   VALUE_ENTRY(BMS_ActualCurrent, "A", 2206)                                              \
   VALUE_ENTRY(BMS_PackPower, "W", 2207)                                                  \
   VALUE_ENTRY(BMS_State, BMS_STATE, 2208)                                                \
   VALUE_ENTRY(BMS_DTC, BMS_DTC_FLAGS, 2209)                                              \
   VALUE_ENTRY(BMS_TimeoutFault, YESNO, 2210)                                             \
   VALUE_ENTRY(BMS_DeltaCellVoltage, "V", 2211)                                           \
   VALUE_ENTRY(BMS_BalancingVoltage, "V", 2212)                                           \
   VALUE_ENTRY(BMS_BalancingActive, "On/Off", 2213)                                       \
   VALUE_ENTRY(BMS_BalancingAnyActive, "On/Off", 2214)                                    \
   VALUE_ENTRY(BMS_PackVoltage, "V", 2215)                                                \
   VALUE_ENTRY(BMS_MaxChargeCurrent, "A", 2216)                                           \
   VALUE_ENTRY(BMS_MaxDischargeCurrent, "A", 2217)                                        \
   VALUE_ENTRY(BMS_ShutdownRequest, "On/Off", 2218)                                       \
   VALUE_ENTRY(BMS_ShutdownReady, "On/Off", 2219)                                         \
   VALUE_ENTRY(BMS_ShutdownAcknowledge, "On/Off", 2220)                                   \
   VALUE_ENTRY(BMS_DataValid, YESNO, 2221)                                                \
   VALUE_ENTRY(BMS_CONT_State, CONT_STATE, 2222)                                          \
   VALUE_ENTRY(BMS_CONT_DTC, CONT_DTC_FLAGS, 2223)                                        \
   VALUE_ENTRY(BMS_CONT_NegativeInput, "On/Off", 2224)                                    \
   VALUE_ENTRY(BMS_CONT_PositiveInput, "On/Off", 2225)                                    \
   VALUE_ENTRY(BMS_CONT_PrechargeInput, "On/Off", 2226)                                   \
   VALUE_ENTRY(BMS_CONT_SupplyVoltageAvailable, "On/Off", 2227)                           \
   PARAM_ENTRY(CAT_HEATER, heater_flap_threshold, "Raw ADC", 0, 4095, 1000, 113)          \
   PARAM_ENTRY(CAT_HEATER, heater_active_manual, "0=Auto, 1=ManualON", 0, 1, 0, 111)      \
   PARAM_ENTRY(CAT_HEATER, heater_contactor_on_delay, "ms", 0, 10000, 2000, 112)          \
   PARAM_ENTRY(CAT_HEATER, heater_thermal_open_timeout, "s", 0, 600, 2, 114)              \
   PARAM_ENTRY(CAT_HEATER, heater_thermal_close_timeout, "s", 0, 600, 5, 115)             \
   VALUE_ENTRY(heater_active, "On/Off", 2130)                                             \
   VALUE_ENTRY(heater_flap_in, "Raw ADC", 2134)                                           \
   VALUE_ENTRY(heater_thermal_switch_in, "0=Overtemp, 1=OK", 2131)                        \
   VALUE_ENTRY(heater_thermal_switch_boot_fault, "0=OK, 1=OvertempOnBoot", 2140)          \
   VALUE_ENTRY(heater_thermal_switch_does_not_open_fault, "0=OK, 1=StuckClosed", 2141)    \
   VALUE_ENTRY(heater_thermal_switch_overheat_fault, "0=OK, 1=TooLongOpen", 2142)         \
   VALUE_ENTRY(heater_contactor_feedback_in, "0=Open, 1=Closed", 2132)                    \
   VALUE_ENTRY(heater_contactor_out, "0=Off, 1=On", 2133)                                 \
   VALUE_ENTRY(heater_contactor_fault, "0=OK, 1=No Feedback, 2=Welded", 2138)             \
   VALUE_ENTRY(heater_fault, "0=OK, 1=Fault", 2143)                                       \
   VALUE_ENTRY(hv_comfort_functions_allowed, "0=No, 1=Yes", 2144)                         \
                                                                                          \
   VALUE_ENTRY(LVDU_ignition_in, "On/Off", 2145)                                          \
   VALUE_ENTRY(LVDU_ready_safety_in, "On/Off", 2146)                                      \
   VALUE_ENTRY(LVDU_vehicle_state, VEHICLE_STATE, 2147)                                   \
   VALUE_ENTRY(LVDU_last_vehicle_state, VEHICLE_STATE, 2148)                              \
   VALUE_ENTRY(LVDU_diagnose_pending, "On/Off", 2149)                                     \
   VALUE_ENTRY(LVDU_12v_battery_voltage, "V", 2155)                                       \
   VALUE_ENTRY(LVDU_vcu_out, "On/Off", 2158)                                              \
   VALUE_ENTRY(LVDU_condition_out, "On/Off", 2159)                                        \
   VALUE_ENTRY(LVDU_ready_out, "On/Off", 2160)                                            \
   VALUE_ENTRY(LVDU_forceVCUsShutdown, "On/Off", 2162)                                    \
   VALUE_ENTRY(LVDU_connectHVcommand, "On/Off", 2163)                                     \
   VALUE_ENTRY(LVDU_hv_contactors_closed, "On/Off", 2168)                                 \
                                                                                          \
   VALUE_ENTRY(eps_state, EPS_STATE, 2164)                                                \
   VALUE_ENTRY(eps_ignition_out, "On/Off", 2165)                                          \
   VALUE_ENTRY(eps_startup_out, "On/Off", 2166)

/***** Enum String definitions *****/
#define OPMODES "0=Off, 1=Run, 2=Precharge, 3=PchFail, 4=Charge"
#define CANSPEEDS "0=125k, 1=250k, 2=500k, 3=800k, 4=1M"
#define CANPERIODS "0=100ms, 1=10ms"
#define CAT_TEST "Testing"
#define CAT_BMS "BMS"
#define CAT_HEATER "Heater"
#define CAT_COMM "Communication"
#define VACUUM_STATE "0=VacuumLow, 1=VacuumOK"
#define VALVE "0=180deg, 1=90deg, 2=Auto"
#define VALVE_STATE "0=180deg, 1=90deg, 2=Transition"
#define VALVE_TARGET "0=180deg, 1=90deg"
#define CAT_VALVE "Tesla Coolant Valve"
#define AUTO_MANUAL "0=Manual, 1=Automatic"
#define CAT_TESLA_COOLANT_PUMP "Tesla Coolant Pump"
#define CAT_EPS "Electric Power Steering"
#define CAT_VACUUM_PUMP "Vacuum Pump"
#define CAN_DEV "0=CAN1, 1=CAN2"
#define CAT_SETUP "General Setup"
#define CAT_TESLA_DCDC "Tesla DCDC"
#define YESNO "0=No, 1=Yes"
#define CAT_LVDU "Low Voltage Distribution"
#define CAT_MLB_SIM "MLB Charger Sim"
#define CHGMODS "0=Off, 1=EXT_DIGI, 2=Volt_Ampera, 3=Leaf_PDM, 4=TeslaOI, 5=Out_lander, 6=Elcon, 7=MGgen2, 8=MLBEvo"
#define HVLM_OPMODE "0=Inactive, 1=Active, 2=Init, 3=Error"
#define HVLM_ACTREQ "0=No Request, 1=Charging, 2=Balancing, 3=AC/Climate"
#define HVLM_ERRORSTATUS "0=No Error, 1=DC-NotOK, 2=AC-NotOK, 3=Interlock, 4=Reserved, 5=Reserved, 6=NoComponentFunction, 7=Init"
#define HVLM_PLUGSTATUS "0=Init, 1=NoPlug, 2=PlugIn, 3=PlugLocked"
#define HVLM_LOADREQ "0=NoRequest, 1=ACCharge, 2=DCCharge, 3=Recharge12V, 4=ACAWCCharge, 5=Reserved, 6=Init, 7=Error"
#define HVLM_CHARGERMODE "0=Standby, 1=ACCharging, 3=DCCharging, 4=Precharge, 5=Fail, 7=Init"
#define HVLM_CHARGESYSSTATE "0=OK, 1=Defective, 2=Incompatible, 3=DCChargeNotPossible"
#define HVLM_CONNECTORLOCK "0=Unlock, 1=Lock, 2=Init, 3=NoRequest"
#define HVLM_CHGREADY "0=NoError, 1=ACNotPossible, 2=DCNotPossible, 3=AC&DCNotPossible"

#define BMS_STATE "0=INIT, 1=OPERATING, 2=FAULT"
#define EPS_STATE "0=OFF, 1=ON, 2=SPOOLUP 3=FAULT"
#define BMS_DTC_FLAGS "0=NONE,1=CAN_SEND_ERROR,2=CAN_INIT_ERROR,4=PACK_FAULT"
#define CONT_STATE "0=INIT,1=OPEN,2=CLOSING_PRECHARGE,3=CLOSING_POSITIVE,4=CLOSED,5=OPENING_POSITIVE,6=OPENING_PRECHARGE,7=FAULT"
#define CONT_DTC_FLAGS "0=NONE,1=NO_SUPPLY,2=NEG_FAULT,4=PRE_FAULT,8=POS_FAULT"
#define VEHICLE_STATE "0=SLEEP,1=STANDBY,2=READY,3=CONDITIONING,4=DRIVE,5=CHARGE,6=ERROR,7=LIMP_HOME"

/***** enums ******/

enum _canperiods
{
   CAN_PERIOD_100MS = 0,
   CAN_PERIOD_10MS,
   CAN_PERIOD_LAST
};

enum modes
{
   MOD_OFF = 0,
   MOD_RUN,
   MOD_PRECHARGE,
   MOD_PCHFAIL,
   MOD_CHARGE,
   MOD_LAST
};

enum _coolant_pump_modes
{
   COOLANT_PUMP_MANUAL = 0,
   COOLANT_PUMP_AUTO,
   COOLANT_PUMP_LAST
};

enum _eps_states
{
   EPS_OFF = 0,
   EPS_ON,
   EPS_SPOOL_UP,
   EPS_FAULT
};

enum ChargeModes
{
   Off = 0,
   EXT_DIGI,
   Volt_Ampera,
   Leaf_PDM,
   TeslaOI,
   Out_lander,
   Elcon,
   MGgen2,
   MLBevo
};

enum can_devices
{
   CAN_DEV1 = 0,
   CAN_DEV2 = 1
};

// Generated enum-string for possible errors
extern const char *errorListString;
