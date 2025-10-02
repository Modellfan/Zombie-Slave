/*
 * This file is part of the ZombieVerter project.
 *
 * Copyright (C) 2024 Mitch Elliott
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

#include <vw_mlb_charger.h>
#include "params.h"

/*CAN send information flow path:

+-------------------------------+           +-------------------------------+
|      [SIMULATION MODE]        |           |    [NON-SIMULATION MODE]      |
|-------------------------------|           |-------------------------------|
|  sim_params (Param)           |           |  zombie_parameters (Param)    |
|  (user/test/GUI values)       |           |  (VCU/user/vehicle setpoints) |
+--------------+----------------+           +--------------+----------------+
               |                                       |
               v                                       v
    +----------------------------+          +-----------------------------+
    |      TagParams()           |          |     TagParams()             |
    |----------------------------|          |-----------------------------|
    | Reads:                     |          | Reads:                      |
    |  - sim_params              |          |  - zombie_parameters        |
    | Writes:                    |          |  - charger_status (CAN in)  |
    |  - battery_status          |          | Writes:                     |
    |  - charger_params          |          |  - battery_status           |
    |  - vehicle_status          |          |  - charger_params           |
    |                            |          |  - vehicle_status           |
    |                            |          |  - zombie_parameters        |
    +--------------+-------------+          |    (Param::Set*)            |
                   |                        +--------------+--------------+
                   |                                       |
    (all status structs populated)         (status structs populated, parameters updated)
                   |                                       |
                   +-------------------+-------------------+
                                       |
                                       v
                        +------------------------------------------+
                        |         Internal Data Structures         |
                        |------------------------------------------|
                        |    battery_status                        |
                        |    charger_params                        |
                        |    vehicle_status                        |
                        +-------------------+----------------------+
                                            |
                                            v
                                +---------------------------+
                                |     CalcValues100ms()     |
                                |---------------------------|
                                | Reads/Writes:             |
                                |  - battery_status         |
                                |  - charger_params         |
                                |  - vehicle_status         |
                                | (computes setpoints,      |
                                |  performs charge logic)   |
                                +------------+--------------+
                                             |
                                             v
                                +---------------------------+
                                |      emulateMLB()         |
                                |---------------------------|
                                | Reads:                    |
                                |  - battery_status         |
                                |  - charger_params         |
                                |  - vehicle_status         |
                                | Writes:                   |
                                |  - mlb_state              |
                                +------------+--------------+
                                             |
                                             v
                                +---------------------------+
                                |        send_can()         |
                                |---------------------------|
                                | Reads: mlb_state          |
                                | Writes: CAN messages      |
                                +------------+--------------+
                                             |
                                             v
                      +-----------------------------------------------+
                      |        CAN Bus: Outgoing                      |
                      |   (Other Vehicle ECUs/Modules)                |
                      +-----------------------------------------------+
*/

/*CAN recieve information flow path:
   +------------+
   |  CAN In    |
   | (messages) |
   +-----+------+
         |
         v
+---------------------+
|    DecodeCAN()      |
|---------------------|
| Decodes messages,   |
| populates           |
| charger_status      |
+----------+----------+
*/

bool VWMLBClass::ControlCharge(bool RunCh, bool ACReq)
{
    (void)ACReq;
    (void)RunCh;
#ifdef MLB_CHARGER_STANDALONE
    /*
     * In simulation mode the activation request is supplied via parameter
     * mlb_chr_sim_Activation_Crg.  Do not override it here.
     */
    charger_params.activate = Param::GetInt(Param::mlb_chr_sim_Activation_Crg);
    return charger_params.activate;
#else
    if (charger_status.HVLM_Plug_Status > 1 && RunCh)
    {
        charger_params.activate = 1;
        return true;
    }
    else
    {
        charger_params.activate = 0;
        return false;
    }
#endif
}

void VWMLBClass::Task10Ms()
{
    static uint8_t counter10ms = 0;

    // stop CAN chatter when requested
    if (vehicle_status.CANQuiet == 1)
    {
        counter10ms = 0;
        return;
    }

    // called every 10 ms
    counter10ms++;

    // --- 10 ms tasks ---
    if (counter10ms % 1 == 0)
    {
        msg191(); // BMS_01   0x191     CRC
    }

    // --- 20 ms tasks (reserved) ---
    if (counter10ms % 2 == 0)
    {
    }

    // --- 40 ms tasks ---
    if (counter10ms % 4 == 0)
    {
        // msg040(); // Airbag_01 - 0x40     CRC
        msg2B1();
    }

    // --- 50 ms tasks ---
    if (counter10ms % 5 == 0)
    {
        msg2AE(); // DCDC_01    0x2AE     CRC
        counter10ms = 0;
    }
}

void VWMLBClass::Task100Ms()
{
    static uint8_t counter100ms = 0;

    TagParams();
    CalcValues100ms();

    // stop CAN chatter when requested
    if (vehicle_status.CANQuiet == 1)
    {
        counter100ms = 0;
        return;
    }

    // called every 100 ms
    counter100ms++;

    // --- 100 ms tasks ---
    if (counter100ms % 1 == 0)
    {
        msg3C0(); //  Klemmen_Status_01   CRC
        msg503(); // HVK_01     0x503     CRC
        msg184(); // ZV_01    0x184       CRC
        msg578(); // BMS_DC_01    0x578   CRC
        msg1A1();
        msg39D();
        msg509();
        msg552();
        msg5AC();
    }

    // --- 200 ms tasks ---
    if (counter100ms % 2 == 0)
    {
        msg1A2(); // ESP_15   0x1A2       CRC
        msg583();
    }

    // --- 500 ms tasks ---
    if (counter100ms % 5 == 0)
    {
        msg5A2(); // BMS_04   0x5A2       CRC
        msg5CA(); // BMS_07   0x5CA       CRC
        msg5CD(); // DCDC_03    0x5CD     CRC
        msg59E();
    }

    // --- 1000 ms tasks ---
    if (counter100ms % 10 == 0)
    {
        msg485();
        msg1A555548();
        msg1A5555AD();
        msg96A955EB();
        msg9A555539();
        msg9A555552();
    }

    // --- 2000 ms tasks ---
    if (counter100ms % 20 == 0)
    {
        msg96A954A6();
        counter100ms = 0;
    }
}

void VWMLBClass::TagParams() // To make code portable between standalone (more params) vs Zombie (basic params)
{

    // copy charger state into values
    Param::SetInt(Param::mlb_chr_DC_Max_ChargePower, charger_status.HVLM_MaxDC_ChargePower);
    Param::SetInt(Param::mlb_chr_DC_Max_ChargeVoltage, charger_status.HVLM_Max_DC_Voltage_DCLS);
    Param::SetInt(Param::mlb_chr_DC_Actual_Current, charger_status.HVLM_Actual_DC_Current_DCLS);
    Param::SetInt(Param::mlb_chr_DC_Max_ChargeCurrent, charger_status.HVLM_Max_DC_Current_DCLS);
    Param::SetInt(Param::mlb_chr_DC_Min_ChargeVoltage, charger_status.HVLM_Min_DC_Voltage_DCLS);
    Param::SetInt(Param::mlb_chr_DC_Min_ChargeCurrent, charger_status.HVLM_Min_DC_Current_DCLS);
    Param::SetInt(Param::mlb_chr_Status_Grid, charger_status.HVLM_EnergyFlowType);
    Param::SetInt(Param::mlb_chr_ChargeManagerMode, charger_status.HVLM_OperationalMode);
    Param::SetInt(Param::mlb_chr_ChargerRequestingHV, charger_status.HVLM_HV_ActivationRequest);
    Param::SetInt(Param::mlb_chr_ChargerErrorStatus, charger_status.HVLM_ChargerErrorStatus);
    Param::SetInt(Param::mlb_chr_PlugStatus, charger_status.HVLM_Plug_Status);
    Param::SetInt(Param::mlb_chr_LoadRequest, charger_status.HVLM_LoadRequest);
    Param::SetInt(Param::mlb_chr_ChargerState, charger_status.mode);
    Param::SetInt(Param::mlb_chr_Charger_AC_Volt_RMS, charger_status.ACvoltage);
    Param::SetInt(Param::mlb_chr_Charger_VoltageOut_HV, charger_status.HVVoltage);
    Param::SetInt(Param::mlb_chr_Charger_CurrentOut_HV, charger_status.current);
    Param::SetInt(Param::mlb_chr_Charger_Temperature, charger_status.temperature);
    Param::SetInt(Param::mlb_chr_ChargerSystemState, charger_status.HVLM_ChargeSystemState);
    Param::SetInt(Param::mlb_chr_Status_LED, charger_status.HVLM_Status_LED);
    Param::SetInt(Param::mlb_chr_MaxCurrent_AC, charger_status.MaxACAmps);
    Param::SetInt(Param::mlb_chr_LockRequest, charger_status.HVLM_RequestConnectorLock);
    Param::SetInt(Param::mlb_chr_Charger_Ready, charger_status.HVLM_ChargeReadyStatus);
    Param::SetInt(Param::mlb_chr_ChargerTemp_Reduction, charger_status.LAD_Reduction_ChargerTemp);
    Param::SetInt(Param::mlb_chr_ChargerCurrent_Reduction, charger_status.LAD_Reduction_Current);
    Param::SetInt(Param::mlb_chr_SocketTemp_Reduction, charger_status.LAD_Reduction_SocketTemp);
    Param::SetInt(Param::mlb_chr_MaxChargerOutput, charger_status.LAD_MaxChargerPower_HV);
    Param::SetInt(Param::mlb_chr_CableCurrentLimit, charger_status.PPLim);
    Param::SetInt(Param::mlb_chr_ControlPilotStatus, charger_status.LAD_ControlPilotStatus);
    Param::SetInt(Param::mlb_chr_LockState, charger_status.LAD_LockFeedback);
    Param::SetInt(Param::mlb_chr_ChargerWarning, charger_status.LAD_ChargerWarning);
    Param::SetInt(Param::mlb_chr_ChargerFault, charger_status.LAD_ChargerFault);
    Param::SetInt(Param::mlb_chr_OutputVolts, charger_status.HVLM_Output_Voltage_HV);
    Param::SetInt(Param::mlb_chr_ActivationState, charger_params.activate);

#ifdef MLB_CHARGER_STANDALONE
    // in standalone mode the vw mlb charger class has no interaction with other class of zombie e.g. via parameters.
    // it is used for testing, and if the vcu should be used only to manage the charger
    // it can be dangerous, because because e.g. single cell values arent considered

    // copy parameters directly from user settable simulation values
    battery_status.SOCx10 = Param::GetInt(Param::mlb_chr_sim_SOC) * 10;
    battery_status.SOC_Targetx10 = Param::GetInt(Param::mlb_chr_sim_SOC_Target) * 10;
    battery_status.BMSMinVolt = Param::GetInt(Param::mlb_chr_sim_BMSMinVolt);
    charger_params.IDCSetpnt = Param::GetInt(Param::mlb_chr_sim_IDCSetpnt);
    charger_params.HVDCSetpnt = Param::GetInt(Param::mlb_chr_sim_HVDCSetpnt);
    battery_status.BMSBattCellSumx10 = Param::GetInt(Param::mlb_chr_sim_BMSBattCellSum) * 10;
    battery_status.BMSMaxVolt = Param::GetInt(Param::mlb_chr_sim_BMSMaxVolt);
    battery_status.BMS_Cell_H_Tempx10 = Param::GetInt(Param::mlb_chr_sim_BMS_Cell_H_Temp) * 10;
    battery_status.BMS_Cell_L_Tempx10 = Param::GetInt(Param::mlb_chr_sim_BMS_Cell_L_Temp) * 10;
    battery_status.BMS_Cell_H_mV = Param::GetInt(Param::mlb_chr_sim_BMS_Cell_H_mV);
    battery_status.BMS_Cell_L_mV = Param::GetInt(Param::mlb_chr_sim_BMS_Cell_L_mV);
    vehicle_status.locked = Param::GetInt(Param::mlb_chr_sim_Lock); // was before mlb_state.ZV_verriegelt_extern_ist = ...
    charger_params.activate = Param::GetInt(Param::mlb_chr_sim_Activation_Crg);
#else
    // normal operation mode with parameters exchange. and reduced parameter set
    battery_status.SOCx10 = int(Param::GetFloat(Param::SOC) * 10);
    battery_status.SOC_Targetx10 = 1000;
    battery_status.CapkWhx10 = int(Param::GetFloat(Param::BattCap) * 10);
    battery_status.BMSMinVolt = Param::GetInt(Param::udcmin);
    charger_params.HVpwr = Param::GetInt(Param::Pwrspnt);
    charger_params.HVcur = Param::GetInt(Param::BMS_ChargeLim);
    charger_params.HVDCSetpnt = Param::GetInt(Param::Voltspnt);
    battery_status.BMSVoltx10 = Param::GetInt(Param::udc);
    battery_status.BMSBattCellSumx10 = Param::GetInt(Param::udc2);
    battery_status.BMSMaxVolt = Param::GetInt(Param::udclim);
    battery_status.BMS_Cell_H_Tempx10 = int(Param::GetFloat(Param::BMS_Tmax) * 10);
    battery_status.BMS_Cell_L_Tempx10 = int(Param::GetFloat(Param::BMS_Tmin) * 10);
    battery_status.BMS_Cell_H_mV = int(Param::GetFloat(Param::BMS_Vmax) * 100);
    battery_status.BMS_Cell_L_mV = int(Param::GetFloat(Param::BMS_Vmin) * 100);
    vehicle_status.locked = Param::GetInt(Param::VehLockSt);

    // backward mapping into ZombieVCU
    Param::SetInt(Param::CableLim, charger_status.MaxACAmps);
    Param::SetInt(Param::AC_Volts, charger_status.ACvoltage);
    Param::SetInt(Param::ChgTemp, charger_status.temperature);
    switch (charger_status.HVLM_Plug_Status)
    {
    case 0:
        Param::SetInt(Param::PlugDet, 0);
        break;
    case 1:
        Param::SetInt(Param::PlugDet, 0);
        break;
    case 2:
        Param::SetInt(Param::PlugDet, 1);
        break;
    case 3:
        Param::SetInt(Param::PlugDet, 1);
        break;
    }

    switch (charger_status.PPLim)
    {
    case 0:
        Param::SetInt(Param::PilotLim, 13);
        break;
    case 1:
        Param::SetInt(Param::PilotLim, 20);
        break;
    case 2:
        Param::SetInt(Param::PilotLim, 32);
        break;
    case 3:
        Param::SetInt(Param::PilotLim, 63);
        break;
    }
#endif
}

void VWMLBClass::CalcValues100ms() // Run to calculate values every 100 ms
{
    // // BMS charge current limit but needs to be power for most AC charger types.
    // if (charger_params.HVcur > 1000)
    // {
    //     charger_params.calcpwr = 12000;
    // }
    // else
    // {
    //     charger_params.calcpwr = charger_params.HVcur * (battery_status.BMSVoltx10 / 10);
    // }

    // charger_params.HVpwr = MIN(charger_params.HVpwr, charger_params.calcpwr);

    // charger_params.IDCSetpnt = charger_params.HVpwr / (battery_status.BMSVoltx10 / 10);

    emulateMLB();
}

//----------------------------------------------------------------------------------------------------------------------------------------------
// Creation of MLB vehicle state
//----------------------------------------------------------------------------------------------------------------------------------------------

void VWMLBClass::emulateMLB()
{
    // Static Set Values - TODO: Roll these up into the CAN msg and eliminate useless variables
    mlb_state.BMS_MaxDischarge_Curr = 1500;
    mlb_state.BMS_MaxCharge_Curr_Offset = 0;
    mlb_state.BMS_Status_ServiceDisconnect = 0;
    mlb_state.BMS_Battdiag = 0;
    mlb_state.BMS_BattEnergy_Wh_HiRes = 0;
    mlb_state.BMS_MaxBattEnergy_Wh_HiRes = 0;
    mlb_state.BMS_ResidualEnergy_Wh = 0;
    mlb_state.BMS_ChargePowerMax = 625;
    mlb_state.BMS_ChargeEnergyCount = 0;
    mlb_state.BMS_EnergyCount = 0;

    // BMS SOC:
    mlb_state.BMS_Batt_Curr = charger_status.current + 2047;
    mlb_state.BMS_SOC = battery_status.SOCx10 / 5;
    mlb_state.BMS_SOC_HiRes = battery_status.SOCx10 * 2;
    mlb_state.BMS_SOC_Kaltstart = battery_status.SOCx10 * 2;
    mlb_state.BMS_Batt_Energy = battery_status.CapkWhx10 * 2;

    // BMS Voltages:
    mlb_state.BMS_Batt_Volt = battery_status.BMSVoltx10 * 4;
    mlb_state.BMS_Batt_Volt_HVterm = battery_status.BMSVoltx10 * 2;
    mlb_state.BMS_BattCell_MV_Max = battery_status.BMS_Cell_H_mV - 1000;
    mlb_state.BMS_BattCell_MV_Min = battery_status.BMS_Cell_L_mV - 1000;

    // BMS Temps:
    mlb_state.BMS_Batt_Temp = (battery_status.BMS_Battery_Tempx10 + 400) / 5;
    mlb_state.BMS_CurrBatt_Temp = (battery_status.BMS_Battery_Tempx10 + 400) / 5;
    mlb_state.BMS_CoolantTemp_Act = (battery_status.BMS_Coolant_Tempx10 + 400) / 5;
    mlb_state.BMS_BattCell_Temp_Max = (battery_status.BMS_Cell_H_Tempx10 + 400) / 5;
    mlb_state.BMS_BattCell_Temp_Min = (battery_status.BMS_Cell_L_Tempx10 + 400) / 5;

    // BMS SOC Limits:
    mlb_state.BMS_Max_Wh = battery_status.CapkWhx10 * 2;
    mlb_state.BMS_SOC_ChargeLim = battery_status.SOC_Targetx10 / 10;
    mlb_state.BMS_max_Grenz_SOC = (battery_status.SOC_Targetx10 - 700) / 10;
    mlb_state.BMS_EnergyReq_Full = ((battery_status.SOC_Targetx10 - battery_status.SOCx10) * battery_status.CapkWhx10) / 2500;

    // BMS Limits Discharge:
    mlb_state.BMS_Min_Batt_Volt = battery_status.BMSMinVolt;
    mlb_state.BMS_Min_Batt_Volt_Discharge = battery_status.BMSMinVolt;

    // BMS Limits Charge:
    mlb_state.BMS_MaxCharge_Curr = charger_params.IDCSetpnt;
    mlb_state.HVEM_SollStrom_HV = (charger_params.IDCSetpnt + 205) * 5;
    mlb_state.BMS_Batt_Max_Volt = charger_params.HVDCSetpnt;
    mlb_state.BMS_Min_Batt_Volt_Charge = battery_status.BMSMinVolt;
    mlb_state.BMS_OpenCircuit_Volts = battery_status.BMSBattCellSumx10 / 10;
    mlb_state.HVEM_MaxSpannung_HV = battery_status.BMSMaxVolt;
    mlb_state.BMS_Faultstatus = battery_status.BMS_Status; // Todo
    mlb_state.BMS_Batt_Ah = (battery_status.CapkWhx10 * 100) / 350;
    mlb_state.BMS_Target_SOC_HiRes = battery_status.SOC_Targetx10 * 2;

    // ESP15
    mlb_state.HMS_Systemstatus = 3;   // 0 "No_function_active" 1 "Hold_active" 2 "Parking_requested" 3 "Parking_active" 4 "Keep parking_active" 5 "Start_active" 6 "Release_request_active" 7 "Release_request_by_driver" 8 "Slipping_detected" 9 "Hold_standby_active" 10 "Start_standby_active" 14 "Init" 15 "Error " ;
    mlb_state.HMS_aktives_System = 6; // 0 "No_System__Init_Error" 1 "Driver request_active" 2 "HMS_internal_active" 3 "ACC_active" 4 "Autohold_active" 5 "HHC_active" 6 "HVLM_active" 7 "Getriebe_aktiv" 8 "EBKV_aktiv" 9 "ParkAssist_aktiv" 10 "ARA_aktiv" 12 "Autonomous_Hold_aktiv" 13 "STA_aktiv " 14 "Motor_aktiv" 15 "EA_aktiv" 16 "VLK_aktiv" ;

    // // Lock Status:
    // if (vehicle_status.locked == 0)
    // {
    //     mlb_state.ZV_verriegelt_soll = 1;
    // }
    // if (vehicle_status.locked == 1)
    // {
    //     mlb_state.ZV_verriegelt_soll = 2;
    // }

    // // Charger Activation State Logic:
    // if (charger_status.HVLM_HV_ActivationRequest == 1)
    // {
    //     mlb_state.HV_Bordnetz_aktiv = true; // Indicates an active high-voltage vehicle electrical system: 0 = Not Active,  1 = Active
    //     // HVK_BMS_Sollmodus = 4;
    //     mlb_state.BMS_IstModus = 4;  // 0=Standby, 1=HV Active (Driving) 2=Balancing 4=AC charge, 6=DC charge, 7=init
    //     mlb_state.BMS_HV_Status = 2; // HV System Voltage Detected  // Voltage Status: 0=Init, 1=NoVoltage, 2=Voltage, 3=Fault & Voltage
    //     mlb_state.HVK_MO_EmSollzustand = 50;
    //     mlb_state.BMS_Charger_Active = 1;
    //     charger_params.HVActiveDelayOff = 20;
    // }

    // if (charger_status.HVLM_HV_ActivationRequest == 0)
    // {
    //     mlb_state.BMS_Charger_Active = 0;
    //     if (charger_params.HVActiveDelayOff >= 1)
    //     {
    //         mlb_state.BMS_HV_Status = 2; // HV No Voltage // Voltage Status: 0=Init, 1=NoVoltage, 2=Voltage, 3=Fault & Voltage
    //         mlb_state.BMS_IstModus = 1;  // 0=Standby, 1=HV Active (Driving) 2=Balancing 4=AC charge, 6=DC charge, 7=init
    //         mlb_state.HVK_BMS_Sollmodus = 1;
    //         mlb_state.HVK_MO_EmSollzustand = 67;
    //         charger_params.HVActiveDelayOff = charger_params.HVActiveDelayOff - 1;
    //     }

    //     if (charger_params.HVActiveDelayOff == 0)
    //     {
    //         //   mlb_state.HV_Bordnetz_aktiv = false; // Indicates an active high-voltage vehicle electrical system: 0 = Not Active,  1 = Active
    //         //   mlb_state.BMS_HV_Status = 1; // HV No Voltage // Voltage Status: 0=Init, 1=NoVoltage, 2=Voltage, 3=Fault & Voltage
    //         //   mlb_state.BMS_IstModus = 0; // 0=Standby, 1=HV Active (Driving) 2=Balancing 4=AC charge, 6=DC charge, 7=init
    //         //   mlb_state.HVK_BMS_Sollmodus = 0;
    //         //   mlb_state.HVK_MO_EmSollzustand = 0;
    //         //   mlb_state.BMS_Batt_Volt = charger_status.HVVoltage*4; // Modify after testing to actual values from BMS/VCU
    //         //   mlb_state.BMS_Batt_Volt_HVterm = charger_status.HVVoltage*2; // Modify after testing to actual values from BMS/VCU

    //         mlb_state.BMS_HV_Status = 2; // Voltage Applied // Voltage Status: 0=Init, 1=NoVoltage, 2=Voltage, 3=Fault & Voltage
    //         mlb_state.BMS_IstModus = 1;  // 0=Standby, 1=HV Active (Driving) 2=Balancing 4=AC charge, 6=DC charge, 7=init
    //         mlb_state.HVK_BMS_Sollmodus = 1;
    //         mlb_state.HVK_MO_EmSollzustand = 67;
    //     }
    // }

    // if (mlb_state.BMS_HV_Status == 1)
    // {
    //     mlb_state.HVK_DCDC_Sollmodus = 2;       // Voltage Status: 0=Init, 1=NoVoltage, 2=Voltage, 3=Fault & Voltage
    //     mlb_state.EM1_Status_Spgfreiheit = 2;   // Voltage Status: 0=Init, 1=NoVoltage, 2=Voltage, 3=Fault & Voltage
    //     mlb_state.HVK_Gesamtst_Spgfreiheit = 2; // Voltage Status: 0=Init, 1=NoVoltage, 2=Voltage, 3=Fault & Voltage
    // }
    // if (mlb_state.BMS_HV_Status == 0)
    // {
    //     mlb_state.HVK_DCDC_Sollmodus = 1;       // Voltage Status: 0=Init, 1=NoVoltage, 2=Voltage, 3=Fault & Voltage
    //     mlb_state.EM1_Status_Spgfreiheit = 1;   // Voltage Status: 0=Init, 1=NoVoltage, 2=Voltage, 3=Fault & Voltage
    //     mlb_state.HVK_Gesamtst_Spgfreiheit = 1; // Voltage Status: 0=Init, 1=NoVoltage, 2=Voltage, 3=Fault & Voltage
    // }

    // switch (charger_params.activate)
    // {
    // case 0:
    //     mlb_state.HVEM_Nachladen_Anf = false; // Request for HV charging with plugged in connector and deactivated charging request
    //     mlb_state.HVK_HVLM_Sollmodus = false; // Requested target mode of the charging manager: 0=Not Enabled, 1=Enabled
    //     mlb_state.BMS_Charger_Active = 0;
    //     break;

    // case 1:
    //     mlb_state.HVEM_Nachladen_Anf = true; // Request for HV charging with plugged in connector and deactivated charging request
    //     mlb_state.HVK_HVLM_Sollmodus = true; // Requested target mode of the charging manager: 0=Not Enabled, 1=Enabled
    //     mlb_state.BMS_Charger_Active = 1;
    //     mlb_state.HVK_BMS_Sollmodus = 4;
    //     break;
    // }

    mlb_state.HVEM_Nachladen_Anf = false; // Request for HV charging with plugged in connector and deactivated charging request
    // HVEM_SollStrom_HV =;  // Target current charging on the HV side
    // HVEM_MaxSpannung_HV =; // Maximum charging voltage to the charger or DC charging station

    if (charger_status.HVLM_Park_Request == 1)
    {
        if (mlb_state.HMS_Systemstatus == 2)
        {
            mlb_state.HMS_Systemstatus = 3;
        }
        else
        {
            mlb_state.HMS_Systemstatus = 2; // 0 "No_function_active" 1 "Hold_active" 2 "Parking_requested" 3 "Parking_active" 4 "Keep parking_active" 5 "Start_active" 6 "Release_request_active" 7 "Release_request_by_driver" 8 "Slipping_detected" 9 "Hold_standby_active" 10 "Start_standby_active" 14 "Init" 15 "Error " ;
        }
        if (mlb_state.HMS_Systemstatus < 1)
        {
            mlb_state.HMS_aktives_System = 6; // 0 "No_System__Init_Error" 1 "Driver request_active" 2 "HMS_internal_active" 3 "ACC_active" 4 "Autohold_active" 5 "HHC_active" 6 "HVLM_active" 7 "Getriebe_aktiv" 8 "EBKV_aktiv" 9 "ParkAssist_aktiv" 10 "ARA_aktiv" 12 "Autonomous_Hold_aktiv" 13 "STA_aktiv " 14 "Motor_aktiv" 15 "EA_aktiv" 16 "VLK_aktiv" ;
        }
        else
        {
            mlb_state.HMS_aktives_System = 0;
        }
        // HMS_Fehlerstatus = false; //0 "No error" 1 "Stopping_not_possible" 2 "Special operating mode_active" 3 "System restriction" 4 "System fault" ;
    }

    if (charger_status.HVLM_HV_ActivationRequest == 1)
    {
        mlb_state.HV_Bordnetz_aktiv = true; // Indicates an active high-voltage vehicle electrical system: 0 = Not Active,  1 = Active
        mlb_state.HVK_BMS_Sollmodus = 4;
        mlb_state.BMS_IstModus = 4;  // 0=Standby, 1=HV Active (Driving) 2=Balancing 4=AC charge, 6=DC charge, 7=init
        mlb_state.BMS_HV_Status = 2; // HV System Voltage Detected  // Voltage Status: 0=Init, 1=NoVoltage, 2=Voltage, 3=Fault & Voltage
        mlb_state.HVK_MO_EmSollzustand = 50;
        mlb_state.BMS_Charger_Active = 1;
        mlb_state.BMS_Batt_Volt = 400 * 4;
        mlb_state.BMS_Batt_Volt_HVterm = 400 * 2;
        if (charger_status.HVVoltage > 250)
        {
            mlb_state.BMS_Batt_Volt = (charger_status.HVVoltage) * 4;
            mlb_state.BMS_Batt_Volt_HVterm = (charger_status.HVVoltage) * 2;
        }
    }

    if (charger_status.HVLM_HV_ActivationRequest == 0)
    {
        mlb_state.HV_Bordnetz_aktiv = false; // Indicates an active high-voltage vehicle electrical system: 0 = Not Active,  1 = Active
        mlb_state.HVK_BMS_Sollmodus = 0;
        mlb_state.BMS_IstModus = 0;  // 0=Standby, 1=HV Active (Driving) 2=Balancing 4=AC charge, 6=DC charge, 7=init
        mlb_state.BMS_HV_Status = 1; // HV No Voltage // Voltage Status: 0=Init, 1=NoVoltage, 2=Voltage, 3=Fault & Voltage
        mlb_state.HVK_MO_EmSollzustand = 50;
        mlb_state.BMS_Charger_Active = 0;
        mlb_state.BMS_Batt_Volt = (charger_status.HVVoltage) * 4;
        mlb_state.BMS_Batt_Volt_HVterm = (charger_status.HVVoltage) * 2;
    }

    if (mlb_state.BMS_HV_Status == 1)
    {
        mlb_state.HVK_DCDC_Sollmodus = 2;       // Voltage Status: 0=Init, 1=NoVoltage, 2=Voltage, 3=Fault & Voltage
        mlb_state.EM1_Status_Spgfreiheit = 2;   // Voltage Status: 0=Init, 1=NoVoltage, 2=Voltage, 3=Fault & Voltage
        mlb_state.HVK_Gesamtst_Spgfreiheit = 2; // Voltage Status: 0=Init, 1=NoVoltage, 2=Voltage, 3=Fault & Voltage
    }
    if (mlb_state.BMS_HV_Status == 0)
    {
        mlb_state.HVK_DCDC_Sollmodus = 1;       // Voltage Status: 0=Init, 1=NoVoltage, 2=Voltage, 3=Fault & Voltage
        mlb_state.EM1_Status_Spgfreiheit = 1;   // Voltage Status: 0=Init, 1=NoVoltage, 2=Voltage, 3=Fault & Voltage
        mlb_state.HVK_Gesamtst_Spgfreiheit = 1; // Voltage Status: 0=Init, 1=NoVoltage, 2=Voltage, 3=Fault & Voltage
    }

    switch (charger_params.activate)
    {
    case 0:                         // Charger Standby
        mlb_state.HVK_HVLM_Sollmodus = false; // Requested target mode of the charging manager: 0=Not Enabled, 1=Enabled
        break;

    case 1: // HV Active - Charger Active
        // HVEM_Nachladen_Anf = true; // Request for HV charging with plugged in connector and deactivated charging request
        mlb_state.HVK_HVLM_Sollmodus = true; // Requested target mode of the charging manager: 0=Not Enabled, 1=Enabled

        break;
    }
    // HMS_Fehlerstatus = false; //0 "No error" 1 "Stopping_not_possible" 2 "Special operating mode_active" 3 "System restriction" 4 "System fault" ;
}

//----------------------------------------------------------------------------------------------------------------------------------------------
// CAN-Messaging
//----------------------------------------------------------------------------------------------------------------------------------------------

void VWMLBClass::SetCanInterface(CanHardware *c)
{
    can = c;
    can->RegisterUserMessage(0x488); // HVLM_06
    can->RegisterUserMessage(0x53C); // HVLM_04
    can->RegisterUserMessage(0x564); // LAD_01
    can->RegisterUserMessage(0x565); // HVLM_03
    can->RegisterUserMessage(0x67E); // LAD_02
}

void VWMLBClass::DecodeCAN(int id, uint32_t data[2])
{
    uint8_t *bytes = (uint8_t *)data; // arrgghhh this converts the two 32bit array into bytes. See comments are useful:) ... Stolen from Zombie, Left comments as they're now famous.
    switch (id)
    {
    case 0x488: // HVLM_06
        charger_status.HVLM_MaxDC_ChargePower = (((bytes[2] & (0x3FU)) << 4) | ((bytes[1] >> 4) & (0x0FU))) * 250;
        charger_status.HVLM_Max_DC_Voltage_DCLS = ((bytes[3] & (0xFFU)) << 2) | ((bytes[2] >> 6) & (0x03U));
        charger_status.HVLM_Actual_DC_Current_DCLS = ((bytes[5] & (0x01U)) << 8) | (bytes[4] & (0xFFU));
        charger_status.HVLM_Max_DC_Current_DCLS = ((bytes[6] & (0x03U)) << 7) | ((bytes[5] >> 1) & (0x7FU));
        charger_status.HVLM_Min_DC_Voltage_DCLS = ((bytes[7] & (0x07U)) << 6) | ((bytes[6] >> 2) & (0x3FU));
        charger_status.HVLM_Min_DC_Current_DCLS = ((bytes[7] >> 3) & (0x1FU));
        break;

    case 0x53C: // HVLM_04
        // HVLM_ParkingHeater_Mode = ((HVLM_04[1] >> 4) & (0x07U));
        // HVLM_StationaryClimat_Timer_Stat = ((HVLM_04[1] >> 7) & (0x01U));
        // HVLM_HVEM_MaxPower = ((HVLM_04[3] & (0x01U)) << 8) | (HVLM_04[2] & (0xFFU));
        charger_status.HVLM_Status_Grid = ((bytes[3] >> 1) & (0x01U));
        // HVLM_BEV_LoadingScreen = ((HVLM_04[3] >> 2) & (0x01U));
        charger_status.HVLM_EnergyFlowType = ((bytes[3] >> 3) & (0x03U));
        // HVLM_VK_ParkingHeaterStatus = ((HVLM_04[3] >> 5) & (0x07U));
        // HVLM_VK_ClimateConditioningStat = (HVLM_04[4] & (0x03U));
        charger_status.HVLM_OperationalMode = ((bytes[4] >> 2) & (0x03U));
        charger_status.HVLM_HV_ActivationRequest = ((bytes[4] >> 4) & (0x03U));
        charger_status.HVLM_ChargerErrorStatus = ((bytes[5] & (0x01U)) << 2) | ((bytes[4] >> 6) & (0x03U));
        charger_status.HVLM_Park_Request = ((bytes[5] >> 1) & (0x07U));
        charger_status.HVLM_Park_Request_Maintain = ((bytes[5] >> 4) & (0x03U));
        // HVLM_AWC_Mode = (HVLM_04[6] & (0x07U));
        charger_status.HVLM_Plug_Status = ((bytes[6] >> 3) & (0x03U));
        charger_status.HVLM_LoadRequest = ((bytes[6] >> 5) & (0x07U));
        charger_status.HVLM_MaxBattChargeCurrent = (bytes[7] & (0xFFU));
        break;

    case 0X564: // LAD_01
        // SerialDEBUG.print("Current charger mode: ");
        // 0=standby,1=AC charging,3=DC charging,4=Precharge,5=Fail,7=init
        charger_status.mode = ((bytes[1] >> 4) & (0x07U));
        charger_status.ACvoltage = ((bytes[2] & (0xFFU)) << 1) | ((bytes[1] >> 7) & (0x01U));
        charger_status.HVVoltage = (((bytes[4] & (0x03U)) << 8) | (bytes[3] & (0xFFU)));
        charger_status.current = ((((bytes[5] & (0x0FU)) << 6) | ((bytes[4] >> 2) & (0x3FU))) * 0.2) - 102;
        charger_status.LAD_Status_Voltage = ((bytes[5] >> 4) & (0x03U));
        charger_status.temperature = bytes[6] - 40;
        charger_status.LAD_PowerLossVal = ((bytes[7] & (0xFFU))) * 20;
        break;

    case 0x565: // HVLM_03
        charger_status.HVLM_HV_StaleTime = ((bytes[0] & (0xFFU))) * 4;
        charger_status.HVLM_ChargeSystemState = (bytes[1] & (0x03U));
        // HVLM_KESSY_KeySearch = ((HVLM_03[1] >> 2) & (0x03U));
        charger_status.HVLM_Status_LED = ((bytes[1] >> 4) & (0x0FU));
        charger_status.MaxACAmps = ((bytes[3] & (0x7FU))) / 2;
        charger_status.HVLM_LG_ChargerTargetMode = ((bytes[3] >> 7) & (0x01U));
        charger_status.HVLM_TankCapReleaseRequest = (bytes[4] & (0x03U));
        charger_status.HVLM_RequestConnectorLock = ((bytes[4] >> 2) & (0x03U));
        charger_status.HVLM_Start_VoltageMeasure_DCLS = ((bytes[4] >> 4) & (0x03U));
        // PnC_Trigger_OBC_cGW = ((HVLM_03[5] & (0x03U)) << 2) | ((HVLM_03[4] >> 6) & (0x03U));
        // HVLM_ReleaseAirConditioning = ((HVLM_03[5] >> 2) & (0x03U));
        charger_status.HVLM_ChargeReadyStatus = ((bytes[6] >> 1) & (0x07U));
        // HVLM_IsolationRequest = ((HVLM_03[6] >> 5) & (0x01U));
        charger_status.HVLM_Output_Voltage_HV = ((bytes[7] & (0xFFU)) << 2) | ((bytes[6] >> 6) & (0x03U));
        break;

    case 0x67E: // LAD_02
        charger_status.LAD_Reduction_ChargerTemp = ((bytes[1] >> 4) & (0x01U));
        charger_status.LAD_Reduction_Current = ((bytes[1] >> 5) & (0x01U));
        charger_status.LAD_Reduction_SocketTemp = ((bytes[1] >> 6) & (0x01U));
        charger_status.LAD_MaxChargerPower_HV = (((bytes[3] & (0x01U)) << 8) | (bytes[2] & (0xFFU))) * 100;
        charger_status.PPLim = (bytes[4] & (0x07U));
        charger_status.LAD_ControlPilotStatus = ((bytes[4] >> 3) & (0x01U));
        charger_status.LAD_LockFeedback = ((bytes[4] >> 4) & (0x01U));
        charger_status.LAD_ChargerCoolingDemand = ((bytes[4] >> 6) & (0x03U));
        // LAD_MaxLadLeistung_HV_Offset = ((LAD_02[7] >> 1) & (0x03U));
        charger_status.LAD_ChargerWarning = ((bytes[7] >> 6) & (0x01U));
        charger_status.LAD_ChargerFault = ((bytes[7] >> 7) & (0x01U));
        break;
    }
}

// Msgs with CRC & Counters:
void VWMLBClass::msg040() // Airbag_01 - 0x40
{
    // CanMsgBuf[0] = 0x00 ;
    // CanMsgBuf[1] = (0x00 | vag_cnt040) ;
    // CanMsgBuf[2] = 0x00 ;
    // CanMsgBuf[3] = 0x00 ;
    // CanMsgBuf[4] = 0x00 ;
    // CanMsgBuf[5] = 0x00 ;
    // CanMsgBuf[6] = 0x00 ;
    // CanMsgBuf[7] = 0x00 ;
    // CanMsgBuf[0] = vag_utils::vw_crc_calc(CanMsgBuf, 8, 0x40);
    // can->Send(0x040, CanMsgBuf, 8);
    // vag_cnt040++;
    // if(vag_cnt040>0x0f) vag_cnt040=0x00;
}

void VWMLBClass::msg184() // ZV_01   0x184
{
    uint8_t buf[8]{};
    buf[1] = ((0x00 & 0x0F) | ((mlb_state.ZV_FT_verriegeln & 0x01) << 4) |
              ((mlb_state.ZV_FT_entriegeln & 0x01) << 5) | ((mlb_state.ZV_BT_verriegeln & 0x01) << 6) |
              ((mlb_state.ZV_BT_entriegeln & 0x01) << 7)) |
             vag_cnt184;
    buf[7] = ((0x00 >> 5) & 0x3F) | ((mlb_state.ZV_entriegeln_Anf & 0x01) << 6);
    buf[0] = vag_utils::vw_crc_calc(buf, 8, ID_ZV_01);
    can->Send(ID_ZV_01, buf, 8);
    vag_cnt184 = (vag_cnt184 + 1) & 0x0F;
}

void VWMLBClass::msg191() // BMS_01   0x191
{
    uint8_t buf[8]{};
    buf[1] = ((mlb_state.BMS_Batt_Curr & 0x0F) << 4) | vag_cnt191;
    buf[2] = (mlb_state.BMS_Batt_Curr >> 4) & 0xFF;
    buf[3] = mlb_state.BMS_Batt_Volt & 0xFF;
    buf[4] = ((mlb_state.BMS_Batt_Volt >> 8) & 0x0F) | ((mlb_state.BMS_Batt_Volt_HVterm & 0x0F) << 4);
    buf[5] = ((mlb_state.BMS_Batt_Volt_HVterm >> 4) & 0x7F) | ((mlb_state.BMS_SOC_HiRes & 0x01) << 7);
    buf[6] = (mlb_state.BMS_SOC_HiRes >> 1) & 0xFF;
    buf[7] = (mlb_state.BMS_SOC_HiRes >> 9) & 0x03;
    buf[0] = vag_utils::vw_crc_calc(buf, 8, ID_BMS_01);
    can->Send(ID_BMS_01, buf, 8);
    vag_cnt191 = (vag_cnt191 + 1) & 0x0F;
}

void VWMLBClass::msg1A2() // ESP_15   0x1A2
{
    uint8_t buf[8]{};
    buf[1] = vag_cnt1A2;
    buf[4] = (mlb_state.HMS_Systemstatus & 0x0F) << 4;
    buf[5] = (mlb_state.HMS_aktives_System & 0x1F) << 3;
    buf[6] = (mlb_state.HMS_Fehlerstatus & 0x07) << 2;
    buf[0] = vag_utils::vw_crc_calc(buf, 8, ID_ESP_15);
    can->Send(ID_ESP_15, buf, 8);
    vag_cnt1A2 = (vag_cnt1A2 + 1) & 0x0F;
}

void VWMLBClass::msg2AE() // DCDC_01    0x2AE
{
    uint8_t buf[8]{};
    buf[1] = vag_cnt2AE;
    buf[7] = 0xA8; // 13.8V output
    buf[0] = vag_utils::vw_crc_calc(buf, 8, ID_DCDC_01);
    can->Send(ID_DCDC_01, buf, 8);
    vag_cnt2AE = (vag_cnt2AE + 1) & 0x0F;
}

void VWMLBClass::msg503() // HVK_01     0x503
{
    uint8_t buf[8]{};
    buf[1] = vag_cnt503;
    buf[2] = mlb_state.HVK_MO_EmSollzustand & 0xFF;
    buf[3] = (mlb_state.HVK_BMS_Sollmodus & 0x07) | ((mlb_state.HVK_DCDC_Sollmodus & 0x07) << 3);
    buf[4] = (mlb_state.HVK_HVLM_Sollmodus & 0x07) << 4;
    buf[5] = (mlb_state.HV_Bordnetz_aktiv & 0x01) << 1 | ((mlb_state.HVK_Gesamtst_Spgfreiheit & 0x03) << 3);
    buf[0] = vag_utils::vw_crc_calc(buf, 8, ID_HVK_01);
    can->Send(ID_HVK_01, buf, 8);
    vag_cnt503 = (vag_cnt503 + 1) & 0x0F;
}

void VWMLBClass::msg578() // BMS_DC_01    0x578
{
    uint8_t buf[8]{};
    buf[1] = vag_cnt578;
    buf[0] = vag_utils::vw_crc_calc(buf, 8, ID_BMS_DC_01);
    can->Send(ID_BMS_DC_01, buf, 8);
    vag_cnt578 = (vag_cnt578 + 1) & 0x0F;
}

void VWMLBClass::msg5A2() // BMS_04   0x5A2
{
    uint8_t buf[8]{};
    buf[1] = ((mlb_state.BMS_Status_ServiceDisconnect & 0x01) << 5) |
             ((mlb_state.BMS_HV_Status & 0x03) << 6) | vag_cnt5A2;
    buf[2] = ((mlb_state.BMS_IstModus & 0x07) << 1) | ((mlb_state.BMS_Faultstatus & 0x07) << 4) |
             ((mlb_state.BMS_Batt_Ah & 0x01) << 7);
    buf[3] = (mlb_state.BMS_Batt_Ah >> 1) & 0xFF;
    buf[4] = (mlb_state.BMS_Batt_Ah >> 9) & 0x03;
    buf[6] = (mlb_state.BMS_Target_SOC_HiRes & 0x07) << 5;
    buf[7] = (mlb_state.BMS_Target_SOC_HiRes >> 3) & 0xFF;
    buf[0] = vag_utils::vw_crc_calc(buf, 8, ID_BMS_04);
    can->Send(ID_BMS_04, buf, 8);
    vag_cnt5A2 = (vag_cnt5A2 + 1) & 0x0F;
}

void VWMLBClass::msg5CA() // BMS_07   0x5CA
{
    uint8_t buf[8]{};
    buf[1] = vag_cnt5CA;
    buf[2] = (mlb_state.BMS_Batt_Energy & 0x0F) << 4;
    buf[3] = ((mlb_state.BMS_Batt_Energy >> 4) & 0x7F) |
             ((mlb_state.BMS_Charger_Active & 0x01) << 7);
    buf[4] = (mlb_state.BMS_Battdiag & 0x07) |
             ((mlb_state.BMS_Freig_max_Perf & 0x03) << 3) |
             ((mlb_state.BMS_Balancing_Active & 0x03) << 6);
    buf[5] = mlb_state.BMS_Max_Wh & 0xFF;
    buf[6] = (mlb_state.BMS_Max_Wh >> 8) & 0x07;
    buf[7] = (mlb_state.BMS_RIso_Ext >> 2) & 0xFF;
    buf[0] = vag_utils::vw_crc_calc(buf, 8, ID_BMS_07);
    can->Send(ID_BMS_07, buf, 8);
    vag_cnt5CA = (vag_cnt5CA + 1) & 0x0F;
}

void VWMLBClass::msg5CD() // DCDC_03    0x5CD
{
    uint8_t buf[8]{};
    buf[1] = vag_cnt5CD;
    buf[2] = (mlb_state.DC_IstModus_02 & 0x07) << 5;
    buf[0] = vag_utils::vw_crc_calc(buf, 8, ID_DCDC_03);
    can->Send(ID_DCDC_03, buf, 8);
    vag_cnt5CD = (vag_cnt5CD + 1) & 0x0F;
}

void VWMLBClass::msg3C0() // Klemmen_Status_01
{
    uint8_t buf[8]{};
    buf[1] = vag_cnt3C0;
    buf[2] = (mlb_state.ZAS_Kl_S & 0x01) | ((mlb_state.ZAS_Kl_15 & 0x01) << 1) | ((mlb_state.ZAS_Kl_X & 0x01) << 2) |
             ((mlb_state.ZAS_Kl_50_Startanforderung & 0x01) << 3);
    buf[0] = vag_utils::vw_crc_calc(buf, 8, 0x3C0);
    can->Send(0x3C0, buf, 8);
    vag_cnt3C0 = (vag_cnt3C0 + 1) & 0x0F;
}

void VWMLBClass::msg1A1() // BMS_02 0x1A1
{
    uint8_t buf[8]{};
    buf[1] = (mlb_state.BMS_MaxCharge_Curr_Offset & 0x0F) | ((mlb_state.BMS_MaxDischarge_Curr & 0x0F) << 4);
    buf[2] = ((mlb_state.BMS_MaxDischarge_Curr >> 4) & 0x7F) | ((mlb_state.BMS_MaxCharge_Curr & 0x01) << 7);
    buf[3] = (mlb_state.BMS_MaxCharge_Curr >> 1) & 0xFF;
    buf[4] = ((mlb_state.BMS_MaxCharge_Curr >> 9) & 0x03) | ((mlb_state.BMS_Min_Batt_Volt & 0x3F) << 2);
    buf[5] = ((mlb_state.BMS_Min_Batt_Volt >> 6) & 0x0F) | ((mlb_state.BMS_Min_Batt_Volt_Discharge & 0x0F) << 4);
    buf[6] = ((mlb_state.BMS_Min_Batt_Volt_Discharge >> 4) & 0x3F) | ((mlb_state.BMS_Min_Batt_Volt_Charge & 0x03) << 6);
    buf[7] = (mlb_state.BMS_Min_Batt_Volt_Charge >> 2) & 0xFF;
    can->Send(ID_BMS_02, buf, 8);
}

void VWMLBClass::msg2B1() // MSG_TME_02   0x2B1
{
    uint8_t buf[8]{};
    can->Send(0x2B1, buf, 8);
}

void VWMLBClass::msg39D() // BMS_03 0x39D
{
    uint8_t buf[8]{};
    buf[0] = mlb_state.BMS_OpenCircuit_Volts & 0xFF;
    buf[1] = ((mlb_state.BMS_OpenCircuit_Volts >> 8) & 0x03) | ((mlb_state.BMS_Batt_Max_Volt & 0x0F) << 4);
    buf[2] = ((mlb_state.BMS_Batt_Max_Volt >> 4) & 0x3F) | ((mlb_state.BMS_MaxDischarge_Curr & 0x03) << 6);
    buf[3] = (mlb_state.BMS_MaxDischarge_Curr >> 2) & 0xFF;
    buf[4] = ((mlb_state.BMS_MaxDischarge_Curr >> 10) & 0x01) | ((mlb_state.BMS_MaxCharge_Curr & 0x7F) << 1);
    buf[5] = ((mlb_state.BMS_MaxCharge_Curr >> 7) & 0x0F) | ((mlb_state.BMS_Min_Batt_Volt_Discharge & 0x0F) << 4);
    buf[6] = ((mlb_state.BMS_Min_Batt_Volt_Discharge >> 4) & 0x3F) | ((mlb_state.BMS_Min_Batt_Volt_Charge & 0x03) << 6);
    buf[7] = (mlb_state.BMS_Min_Batt_Volt_Charge >> 2) & 0xFF;
    can->Send(ID_BMS_03, buf, 8);
}

void VWMLBClass::msg509() // BMS_10 0x509
{
    uint8_t buf[8]{};
    buf[0] = mlb_state.BMS_BattEnergy_Wh_HiRes & 0xFF;
    buf[1] = ((mlb_state.BMS_BattEnergy_Wh_HiRes >> 8) & 0x7F) | ((mlb_state.BMS_MaxBattEnergy_Wh_HiRes & 0x01) << 7);
    buf[2] = (mlb_state.BMS_MaxBattEnergy_Wh_HiRes >> 1) & 0xFF;
    buf[3] = ((mlb_state.BMS_MaxBattEnergy_Wh_HiRes >> 9) & 0x3F) | ((mlb_state.BMS_SOC & 0x03) << 6);
    buf[4] = ((mlb_state.BMS_SOC >> 2) & 0x3F) | ((mlb_state.BMS_ResidualEnergy_Wh & 0x03) << 6);
    buf[5] = (mlb_state.BMS_ResidualEnergy_Wh >> 2) & 0xFF;
    buf[6] = ((mlb_state.BMS_ResidualEnergy_Wh >> 10) & 0x03) | ((0x64 & 0x3F) << 2);
    buf[7] = ((0x64 >> 6) & 0x01) | ((0x64 & 0x7F) << 1);
    can->Send(ID_BMS_10, buf, 8);
}

void VWMLBClass::msg552() // HVEM_05 0x552
{
    uint8_t buf[8]{};
    buf[0] = 0;
    buf[1] = (mlb_state.HVEM_NVNachladen_Energie & 0x0F) << 4;
    buf[2] = (mlb_state.HVEM_NVNachladen_Energie >> 4) & 0x0F;
    buf[4] = (mlb_state.HVEM_Nachladen_Anf & 0x01) | ((mlb_state.HVEM_SollStrom_HV & 0x7F) << 1);
    buf[5] = ((mlb_state.HVEM_SollStrom_HV >> 7) & 0x0F) | ((mlb_state.HVEM_MaxSpannung_HV & 0x0F) << 4);
    buf[6] = (mlb_state.HVEM_MaxSpannung_HV >> 4) & 0x3F;
    can->Send(ID_HVEM_05, buf, 8);
}

void VWMLBClass::msg5AC() // HVEM_02 0x5AC
{
    uint8_t buf[8] = {0xFF, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    can->Send(ID_HVEM_02, buf, 8);
}

void VWMLBClass::msg583() // ZV_02 0x583
{
    uint8_t buf[8]{};
    buf[2] = (mlb_state.ZV_verriegelt_intern_ist & 0x01) |
             ((mlb_state.ZV_verriegelt_extern_ist & 0x01) << 1) |
             ((mlb_state.ZV_verriegelt_intern_soll & 0x01) << 2) |
             ((mlb_state.ZV_verriegelt_extern_soll & 0x01) << 3);
    buf[7] = (mlb_state.ZV_verriegelt_soll & 0x03) << 6;
    can->Send(ID_ZV_02, buf, 8);
}

void VWMLBClass::msg59E() // BMS_06 0x59E
{
    uint8_t buf[8]{};
    buf[2] = mlb_state.BMS_Batt_Temp & 0xFF;
    buf[3] = mlb_state.BMS_CurrBatt_Temp & 0xFF;
    buf[7] = mlb_state.BMS_CoolantTemp_Act & 0xFF;
    can->Send(ID_BMS_06, buf, 8);
}

void VWMLBClass::msg485() // NavData_02 0x485
{
    uint8_t buf[8]{};
    buf[4] = mlb_state.UnixTime & 0xFF;
    buf[5] = (mlb_state.UnixTime >> 8) & 0xFF;
    buf[6] = (mlb_state.UnixTime >> 16) & 0xFF;
    buf[7] = (mlb_state.UnixTime >> 24) & 0xFF;
    can->Send(ID_NAVDATA_02, buf, 8);
}

void VWMLBClass::msg1A555548() // ORU_01 0x1A555548
{
    uint8_t buf[8]{};
    can->Send(ID_ORU_01, buf, 8);
}

void VWMLBClass::msg1A5555AD() // Authentic_Time_01 0x1A5555AD
{
    uint8_t buf[8]{};
    buf[4] = mlb_state.UnixTime & 0xFF;
    buf[5] = (mlb_state.UnixTime >> 8) & 0xFF;
    buf[6] = (mlb_state.UnixTime >> 16) & 0xFF;
    buf[7] = (mlb_state.UnixTime >> 24) & 0xFF;
    can->Send(ID_AUTHENTIC_TIME_01, buf, 8);
}

void VWMLBClass::msg96A955EB() // BMS_09 0x96A955EB
{
    uint8_t buf[8]{};
    buf[2] = ((mlb_state.BMS_HV_Auszeit_Status & 0x03) << 5) | ((mlb_state.BMS_HV_Auszeit & 0x01) << 7);
    buf[3] = (mlb_state.BMS_HV_Auszeit >> 1) & 0xFF;
    buf[4] = mlb_state.BMS_Kapazitaet & 0xFF;
    buf[5] = ((mlb_state.BMS_Kapazitaet >> 8) & 0x07) | ((mlb_state.BMS_SOC_Kaltstart & 0x1F) << 3);
    buf[6] = ((mlb_state.BMS_SOC_Kaltstart >> 5) & 0x3F) | ((mlb_state.BMS_max_Grenz_SOC & 0x03) << 6);
    buf[7] = ((mlb_state.BMS_max_Grenz_SOC >> 2) & 0x07) | ((mlb_state.BMS_min_Grenz_SOC & 0x1F) << 3);
    can->Send(ID_BMS_09, buf, 8);
}

void VWMLBClass::msg96A954A6() // BMS_11 0x96A954A6
{
    uint8_t buf[8]{};
    buf[3] = mlb_state.BMS_BattCell_Temp_Max & 0xFF;
    buf[4] = mlb_state.BMS_BattCell_Temp_Min & 0xFF;
    buf[5] = mlb_state.BMS_BattCell_MV_Max & 0xFF;
    buf[6] = ((mlb_state.BMS_BattCell_MV_Max >> 8) & 0x0F) | ((mlb_state.BMS_BattCell_MV_Min & 0x0F) << 4);
    buf[7] = (mlb_state.BMS_BattCell_MV_Min >> 4) & 0xFF;
    can->Send(ID_BMS_11, buf, 8);
}

void VWMLBClass::msg9A555539() // BMS_16 0x9A555539
{
    uint8_t buf[8]{};
    can->Send(ID_BMS_16, buf, 8);
}

void VWMLBClass::msg9A555552() // BMS_27 0x9A555552
{
    uint8_t buf[8]{};
    buf[3] = (mlb_state.BMS_SOC_ChargeLim & 0x3F) << 2;
    buf[4] = ((mlb_state.BMS_SOC_ChargeLim >> 6) & 0x01) | ((mlb_state.BMS_EnergyCount & 0x0F) << 1) |
             ((mlb_state.BMS_EnergyReq_Full & 0x07) << 5);
    buf[5] = (mlb_state.BMS_EnergyReq_Full >> 3) & 0xFF;
    buf[6] = mlb_state.BMS_ChargePowerMax & 0xFF;
    buf[7] = ((mlb_state.BMS_ChargePowerMax >> 8) & 0x0F) | ((mlb_state.BMS_ChargeEnergyCount & 0x0F) << 4);
    can->Send(ID_BMS_27, buf, 8);
}
