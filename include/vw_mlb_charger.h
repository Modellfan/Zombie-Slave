
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
#ifndef vw_mlb_charger_h
#define vw_mlb_charger_h

#include "chargerhw.h"
#include "canhardware.h"
#include <stdint.h>
#include "my_fp.h"
#include "params.h"
#include "my_math.h"
#include "stm32_can.h"
#include "CANSPI.h"
#include "vag_utils.h"

struct VehicleStatus {
    bool locked = false;
    bool CANQuiet = false;
};

struct ChargerStatus {
    uint16_t ACvoltage;
    uint16_t HVVoltage;
    int8_t temperature;
    uint8_t mode; // 0=Standby, 1=AC charging, 3=DC charging, 4=Precharge, 5=Fail, 7=Init
    uint16_t current;
    uint8_t MaxACAmps;
    uint8_t PPLim;
    uint32_t HVLM_MaxDC_ChargePower;       // maximum DC charging power
    uint16_t HVLM_Max_DC_Voltage_DCLS;     // maximum DC charging voltage
    uint16_t HVLM_Actual_DC_Current_DCLS;  // actual DC charging current
    uint16_t HVLM_Max_DC_Current_DCLS;     // maximum DC charging current
    uint16_t HVLM_Min_DC_Voltage_DCLS;     // minimum DC charging voltage
    uint16_t HVLM_Min_DC_Current_DCLS;     // minimum DC charging current
    uint8_t HVLM_Status_Grid;              // connected to power grid
    uint8_t HVLM_EnergyFlowType;           // where the energy is flowing
    uint8_t HVLM_OperationalMode;          // 0=Inactive, 1=Active, 2=Init, 3=Error
    uint8_t HVLM_HV_ActivationRequest;     // 0=No Request, 1=Charging, 2=Balancing, 3=AC/Climate
    uint8_t HVLM_ChargerErrorStatus;       // 0=No Error, 1=DC-NotOK, 2=AC-NotOK, 3=Interlock, 4-5=Reserved, 6=No Component Function, 7=Init
    uint8_t HVLM_Park_Request;             // request to lock the drive train
    uint8_t HVLM_Park_Request_Maintain;    // keep the drive train locked
    uint8_t HVLM_Plug_Status;              // 0=Init, 1=No Plug, 2=Plug In, 3=Plug Locked
    uint8_t HVLM_LoadRequest;              // 0=No Request, 1=AC Charge, 2=DC Charge, 3=Recharge 12V, 4=AC AWC Charge, 5=Reserved, 6=Init, 7=Error
    uint8_t HVLM_MaxBattChargeCurrent;     // recommended HV battery charging current
    uint8_t LAD_Mode;                      // charger operating mode
    uint16_t LAD_AC_Volt_RMS;              // AC grid voltage (RMS)
    uint16_t LAD_VoltageOut_HV;            // charger output voltage
    uint16_t LAD_CurrentOut_HV;            // charger output current
    uint8_t LAD_Status_Voltage;
    uint16_t LAD_Temperature;              // charger temperature
    uint16_t LAD_PowerLossVal;             // charger power loss
    uint16_t HVLM_HV_StaleTime;            // time between HV off and on
    uint8_t HVLM_ChargeSystemState;        // 0=OK, 1=Defective, 2=Incompatible, 3=DC Charge not possible
    uint8_t HVLM_Status_LED;               // status of the charging LED
    uint8_t HVLM_MaxCurrent_AC;            // max AC current
    bool HVLM_LG_ChargerTargetMode;        // 0=Standby, 1=Mains Charging
    uint8_t HVLM_TankCapReleaseRequest;    // 0=No Release, 1=Release, 2=Init, 3=Error
    uint8_t HVLM_RequestConnectorLock;     // 0=Unlock, 1=Lock, 2=Init, 3=No Request
    uint8_t HVLM_Start_VoltageMeasure_DCLS;// 0=Inactive, 1=DCLS With Diode, 2=DCLS Without Diode, 3=Reserved
    uint8_t HVLM_ChargeReadyStatus;        // 0=No Error, 1=AC Not Possible, 2=DC Not Possible, 3=AC & DC Not Possible
    uint16_t HVLM_Output_Voltage_HV;       // charger output voltage / DC station voltage
    bool LAD_Reduction_ChargerTemp;        // reduced due to charger temp
    bool LAD_Reduction_Current;            // regulated due to current or voltage
    bool LAD_Reduction_SocketTemp;         // reduced due to socket temperature
    uint16_t LAD_MaxChargerPower_HV;       // max charger power
    uint8_t LAD_PRX_CableCurrentLimit;     // AC current limit from PRX cable
    bool LAD_ControlPilotStatus;           // status of control pilot monitoring
    bool LAD_LockFeedback;                 // status of connector lock feedback
    uint8_t LAD_ChargerCoolingDemand;      // cooling demand of the charger
    bool LAD_ChargerWarning;               // warning flag
    bool LAD_ChargerFault;                 // fault flag
};

struct ChargerControl {
    uint16_t HVDCSetpnt;
    uint16_t IDCSetpnt;
    uint16_t HVpwr = 0;
    uint16_t HVcur = 0;
    uint16_t calcpwr = 0;
    bool activate;
    uint8_t HVActiveDelayOff;
};

struct BatteryStatus {
    uint16_t SOCx10;         // SOC of battery, with implied decimal place
    uint16_t SOC_Targetx10;  // target SOC of battery, with implied decimal place
    uint16_t CapkWhx10 = 365;      // usable energy content of the HV battery
    uint16_t BattkWhx10 = 273;     // current energy content of the HV battery
    uint16_t BMSVoltx10 = 4100;     // BMS voltage of battery
    uint16_t BMSCurrx10 =1256;     // BMS current of battery, with implied decimal place
    uint16_t BMSMaxVolt=400;     // BMS maximum battery voltage
    uint16_t BMSMinVolt=0;     // BMS minimum battery voltage
    uint16_t BMSMaxChargeCurr=300;  // BMS maximum charge current
    uint16_t BMSBattCellSumx10=0;
    uint16_t BMSCellAhx10 = 1080;
    uint8_t HV_Status;       // 0=Init (no function), 1=BMS HV free (<20V), 2=BMS HV active (>=25V), 3=Error
    uint8_t BMS_Status;      // 0 "Component_OK" 1 "Limited_CompFct_Isoerror_I" 2 "Limited_CompFct_Isoerror_II" 3 "Limited_CompFct_Interlock" 4 "Limited_CompFct_SD" 5 "Limited_CompFct_Powerred" 6 "No_component_function" 7 "Init"
    uint8_t BMS_Mode;        // 0 "HV_Off" 1 "Driving HV Active" 2 "Balancing" 3 "External Charger" 4 "AC Charging" 5 "Battery Fault" 6 "DC Charging" 7 "Init"
    uint16_t BMS_Battery_Tempx10 = 242;
    uint16_t BMS_Coolant_Tempx10 = 201;
    uint16_t BMS_Cell_H_Tempx10 = 290;
    uint16_t BMS_Cell_L_Tempx10 = 220;
    uint16_t BMS_Cell_H_mV = 3850;
    uint16_t BMS_Cell_L_mV = 3750;
    bool HVIL_Open = false;
};

struct MLB_State {
    uint32_t UnixTime{};
    uint16_t BMS_Batt_Curr{};
    uint16_t BMS_Batt_Volt{};
    uint16_t BMS_Batt_Volt_HVterm{};
    uint16_t BMS_SOC_HiRes{};
    uint16_t BMS_MaxDischarge_Curr{};
    uint16_t BMS_Min_Batt_Volt{};
    uint16_t BMS_Min_Batt_Volt_Discharge{};
    uint16_t BMS_MaxCharge_Curr{};
    uint16_t BMS_MaxCharge_Curr_Offset{};
    uint16_t BMS_Batt_Max_Volt{};
    uint16_t BMS_Min_Batt_Volt_Charge{};
    uint16_t BMS_OpenCircuit_Volts{};
    bool BMS_Status_ServiceDisconnect{};
    uint8_t BMS_HV_Status{};
    bool BMS_Faultstatus{};
    uint8_t BMS_IstModus{};
    uint16_t BMS_Batt_Ah{};
    uint16_t BMS_Target_SOC_HiRes{};
    uint16_t BMS_Batt_Temp{};
    uint16_t BMS_CurrBatt_Temp{};
    uint16_t BMS_CoolantTemp_Act{};
    uint16_t BMS_Batt_Energy{};
    uint16_t BMS_Max_Wh{};
    uint16_t BMS_BattEnergy_Wh_HiRes{};
    uint16_t BMS_MaxBattEnergy_Wh_HiRes{};
    uint16_t BMS_SOC{};
    uint16_t BMS_ResidualEnergy_Wh{};
    uint16_t BMS_SOC_ChargeLim{};
    uint16_t BMS_EnergyCount{};
    uint16_t BMS_EnergyReq_Full{};
    uint16_t BMS_ChargePowerMax{};
    uint16_t BMS_ChargeEnergyCount{};
    uint16_t BMS_BattCell_Temp_Max{};
    uint16_t BMS_BattCell_Temp_Min{};
    uint16_t BMS_BattCell_MV_Max{};
    uint16_t BMS_BattCell_MV_Min{};
    bool HVEM_Nachladen_Anf{};
    uint16_t HVEM_SollStrom_HV{};
    uint16_t HVEM_MaxSpannung_HV{};
    uint8_t HMS_Systemstatus{};
    uint8_t HMS_aktives_System{};
    bool HMS_Fehlerstatus{};
    uint8_t HVK_HVLM_Sollmodus{}; // requested target mode of the charging manager: 0=Not Enabled, 1=Enabled
    bool HV_Bordnetz_aktiv{};     // high-voltage vehicle electrical system active: 0=Not Active, 1=Active
    uint8_t HVK_MO_EmSollzustand{}; // 0 "HvOff" 1 "HvStbyReq" 2 "HvStbyWait" 3 "HvBattOnReq" 4 "HvBattOnWait" 10 "HvOnIdle" 20 "HvOnDrvRdy" 46 "HvAcChPreReq" 47 "HvAcChPreWait" 48 "HvAcChReq" 49 "HvDcChWait" 50 "HvDcCh" 56 "HvDcChPreReq" 57 "HvDcChPreWait" 58 "HvDcChReq" 59 "HvDcChWait" 60 "HvDcCh" 67 "HvChOffReq" 68 "HvChOffWait" 69 "HvOnIdleReq" 70 "HvOnIdleWait" 96 "HvCpntOffReq" 97 "HvCpntOffWait" 98 "HvBattOffReq" 99 "HvBattOffWait" 119 "HvElmOffReq" 120 "HvElmOff"
    uint8_t HVK_BMS_Sollmodus{};    // BMS requested mode: 0 "HV_Off" 1 "HV_On" 3 "AC_Charging_ext" 4 "AC_Charging" 6 "DC_Charging" 7 "Init"
    uint8_t HVK_DCDC_Sollmodus{};   // DC/DC requested mode: 0 "Standby" 1 "HV_On_Precharging" 2 "Step down" 3 "Step up" 4 "Test pulse_12V" 7 "Initialization"
    bool ZV_FT_verriegeln{};
    bool ZV_FT_entriegeln{};
    bool ZV_BT_verriegeln{};
    bool ZV_BT_entriegeln{};
    bool ZV_entriegeln_Anf{};
    bool ZV_verriegelt_intern_ist{};
    bool ZV_verriegelt_extern_ist{};
    bool ZV_verriegelt_intern_soll{};
    bool ZV_verriegelt_extern_soll{};
    uint8_t ZV_verriegelt_soll{};
    bool BMS_Charger_Active{};
    uint16_t BMS_RIso_Ext{4090};
    uint8_t HVK_Gesamtst_Spgfreiheit{};
    uint8_t BMS_Balancing_Active{2};
    uint8_t BMS_Freig_max_Perf{1};
    uint8_t BMS_Battdiag{1};       // battery display diagnostics: 1=Display Battery, 4=Battery OK, 5=Charging, 6=Check Battery
    uint8_t DC_IstModus_02{2};
    uint8_t BMS_HV_Auszeit_Status{1}; // status HV timeout
    uint16_t BMS_HV_Auszeit{25};      // time since last HV activity
    uint16_t BMS_Kapazitaet{1000};    // total energy capacity (aged)
    uint16_t BMS_SOC_Kaltstart{};     // SOC cold
    uint8_t BMS_max_Grenz_SOC{30};    // upper limit of SOC operating strategy (70 offset -> 30 = 100)
    uint8_t BMS_min_Grenz_SOC{15};    // lower limit of SOC operating strategy
    uint8_t EM1_Status_Spgfreiheit{};    // voltage status: 0=Init, 1=NoVoltage, 2=Voltage, 3=Fault & Voltage
    bool ZAS_Kl_S{};                    // key switch inserted
    bool ZAS_Kl_15{};                   // accessory position
    bool ZAS_Kl_X{};                    // run position
    bool ZAS_Kl_50_Startanforderung{};   // start request
    uint8_t HVEM_NVNachladen_Energie{200};
};

class VWMLBClass: public Chargerhw
{
public:
      bool ControlCharge(bool RunCh, bool ACReq);
      void SetCanInterface(CanHardware*);
      void DecodeCAN(int id, uint32_t data[2]);
      void Task10Ms();
      void Task100Ms();


private:
      static constexpr uint32_t ID_ZV_01  = 0x184;
      static constexpr uint32_t ID_BMS_01 = 0x191;
      static constexpr uint32_t ID_BMS_02 = 0x1A1;
      static constexpr uint32_t ID_BMS_03 = 0x39D;
      static constexpr uint32_t ID_DCDC_01 = 0x2AE;
      static constexpr uint32_t ID_BMS_10 = 0x509;
      static constexpr uint32_t ID_HVK_01 = 0x503;
      static constexpr uint32_t ID_BMS_DC_01 = 0x578;
      static constexpr uint32_t ID_HVEM_05 = 0x552;
      static constexpr uint32_t ID_ZV_02  = 0x583;
      static constexpr uint32_t ID_BMS_04 = 0x5A2;
      static constexpr uint32_t ID_BMS_06 = 0x59E;
      static constexpr uint32_t ID_BMS_07 = 0x5CA;
      static constexpr uint32_t ID_DCDC_03 = 0x5CD;
      static constexpr uint32_t ID_HVEM_02 = 0x5AC;
      static constexpr uint32_t ID_NAVDATA_02 = 0x485;
      static constexpr uint32_t ID_ORU_01 = 0x1A555548;
      static constexpr uint32_t ID_AUTHENTIC_TIME_01 = 0x1A5555AD;
      static constexpr uint32_t ID_BMS_09 = 0x96A955EB;
      static constexpr uint32_t ID_BMS_11 = 0x96A954A6;
      static constexpr uint32_t ID_BMS_16 = 0x9A555539;
      static constexpr uint32_t ID_BMS_27 = 0x9A555552;
      static constexpr uint32_t ID_ESP_15 = 0x1A2;
      //void CommandStates();
      void TagParams();
      void CalcValues100ms();
      void msg3C0();
      void msg1A1();      // BMS_02     0x1A1
      void msg2B1();      // MSG_TME_02   0x2B1
      void msg39D();      // BMS_03     0x39D
      void msg485();      // NavData_02 0x485
      void msg509();      // BMS_10     0x509
      void msg552();      // HVEM_05    0x552
      void msg583();      // ZV_02      0x583
      void msg59E();      // BMS_06     0x59E
      void msg5AC();      // HVEM_02    0x5AC
      void msg64F();      // BCM1_04    0x64F
      void msg663();      // NVEM_02    0x663
      void msg1A555548(); // ORU_01     0x1A555548
      void msg1A5555AD(); // Authentic_Time_01   0x1A5555AD
      void msg96A955EB(); // BMS_09     0x96A955EB
      void msg96A954A6(); // BMS_11     0x96A954A6
      void msg9A555539(); // BMS_16     0x9A555539
      void msg9A555552(); // BMS_27     0x9A555552
      void msg040();      // Airbag_01  0x40
      void msg184();      // ZV_01      0x184
      void msg191();      // BMS_01     0x191
      void msg1A2();      // ESP_15   0x1A2
      void msg2AE();      // DCDC_01    0x2AE
      void msg37C();      // EM1_HYB_11    0x37C
      void msg503();      // HVK_01     0x503
      void msg578();      // BMS_DC_01  0x578
      void msg5A2();      // BMS_04     0x5A2
      void msg5CA();      // BMS_07     0x5CA
      void msg5CD();      // DCDC_03    0x5CD
      uint8_t vag_cnt3C0            = 0x00;
      uint8_t vag_cnt184            = 0x00;
      uint8_t vag_cnt191            = 0x00;
      uint8_t vag_cnt1A2            = 0x00;
      uint8_t vag_cnt2AE            = 0x00;
      uint8_t vag_cnt503            = 0x00;
      uint8_t vag_cnt578            = 0x00;
      uint8_t vag_cnt5A2            = 0x00;
      uint8_t vag_cnt5CA            = 0x00;
      uint8_t vag_cnt5CD            = 0x00;

      VehicleStatus vehicle_status;
      ChargerStatus charger_status;
      ChargerControl charger_params;
      BatteryStatus battery_status;

      MLB_State mlb_state{};

      void emulateMLB();

      
};

#endif /* vw_mlb_charger_h */
