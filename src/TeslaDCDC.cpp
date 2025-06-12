/*
 * This file is part of the ZombieVerter project.
 *
 * Copyright (C) 2023 Damien Maguire
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

/* This is an interface for The Tesla GEN2 DCDC converter
 * https://openinverter.org/wiki/Tesla_Model_S/X_DC/DC_Converter
 */

#include "TeslaDCDC.h"
#include "lvdu.h" // for VehicleState enums

 #define TESLA_DCDC_STATUS_ID     0x210
 #define TESLA_DCDC_CMD_ID        0x3D8
 #define TESLA_DCDC_TIMEOUT_TICKS 10  // 10 x 100ms = 1000ms
 
 void TeslaDCDC::SetCanInterface(CanHardware* c)
 {
     if (!c)
         return;
 
     can = c;
     can->RegisterUserMessage(TESLA_DCDC_STATUS_ID);
     timeoutCounter = 0;
 }
 
 void TeslaDCDC::DecodeCAN(int id, uint8_t *data)
 {
     if (id != TESLA_DCDC_STATUS_ID || !data)
         return;
 
     timeoutCounter = 0;
 
     // Byte 0: Fault flags
     bool heaterShorted     = data[0] & (1 << 0);
     bool overTemp          = data[0] & (1 << 1);
     bool outputUndervolt   = data[0] & (1 << 2);
     bool biasFault         = data[0] & (1 << 3);
     bool inputNotOk        = data[0] & (1 << 4);
     bool outputOvervolt    = data[0] & (1 << 5);
     bool currentLimited    = data[0] & (1 << 6);
     bool heaterOpenCircuit = data[0] & (1 << 7);
 
     // Byte 1: Status flags
     bool coolantRequest         = data[1] & (1 << 0);
     bool thermalLimit           = data[1] & (1 << 1);
     bool voltageRegFault        = data[1] & (1 << 2);
     bool calibrationFactorFault = data[1] & (1 << 3);
 
     // Byte 2–5: Scaled measurements
    // Coolant temperature is encoded as a signed byte with 0.5 °C steps and
    // an offset of +40 °C.  Use sign-extension to correctly handle negative
    // values.
    float coolantTemp   = ((data[2] - (2 * (data[2] & 0x80))) * 0.5f) + 40.0f;
     float inputPower    = data[3] * 16.0f;
     float outputCurrent = static_cast<float>(data[4]);
     float outputVoltage = data[5] * 0.1f;
 
     // Set values
     Param::SetFloat(Param::dcdc_coolant_temp, coolantTemp);
     Param::SetFloat(Param::dcdc_input_power, inputPower);
     Param::SetFloat(Param::dcdc_output_current, outputCurrent);
     Param::SetFloat(Param::dcdc_output_voltage, outputVoltage);
 
     // Set fault flags
     Param::SetInt(Param::dcdc_fault_heater_shorted, heaterShorted);
     Param::SetInt(Param::dcdc_fault_overtemp, overTemp);
     Param::SetInt(Param::dcdc_fault_undervolt, outputUndervolt);
     Param::SetInt(Param::dcdc_fault_bias, biasFault);
     Param::SetInt(Param::dcdc_fault_input_not_ok, inputNotOk);
     Param::SetInt(Param::dcdc_fault_overvolt, outputOvervolt);
     Param::SetInt(Param::dcdc_fault_current_limited, currentLimited);
     Param::SetInt(Param::dcdc_fault_heater_open, heaterOpenCircuit);
 
     // Set status flags
     Param::SetInt(Param::dcdc_status_coolant_request, coolantRequest);
     Param::SetInt(Param::dcdc_status_thermal_limit, thermalLimit);
     Param::SetInt(Param::dcdc_status_voltage_reg_fault, voltageRegFault);
     Param::SetInt(Param::dcdc_status_calibration_fault, calibrationFactorFault);
 
     // Clear timeout flag
     Param::SetInt(Param::dcdc_fault_timeout, 0);
 
     // Fault aggregation (timeout is not active here)
     bool anyFault = heaterShorted || overTemp || outputUndervolt || biasFault ||
                     inputNotOk || outputOvervolt || currentLimited || heaterOpenCircuit ||
                     thermalLimit || voltageRegFault || calibrationFactorFault;
 
     Param::SetInt(Param::dcdc_fault_any, anyFault);
 }
 
 void TeslaDCDC::Task100Ms()
 {
    //  int opmode = Param::GetInt(Param::opmode);
    float DCSetVal = Param::GetFloat(Param::dcdc_voltage_setpoint);
    uint8_t bytes[8] = {0};

    // Timeout handling
    timeoutCounter++;
    if (timeoutCounter > TESLA_DCDC_TIMEOUT_TICKS)
    {
        Param::SetInt(Param::dcdc_fault_timeout, 1);
        Param::SetInt(Param::dcdc_fault_any, 1);  // Include timeout as a fault
    }

    // Check vehicle state - only send commands in READY, DRIVE, CONDITIONING or LIMP_HOME
    int vehicleState = Param::GetInt(Param::LVDU_vehicle_state);
    if (vehicleState != STATE_READY && vehicleState != STATE_DRIVE &&
        vehicleState != STATE_CONDITIONING && vehicleState != STATE_LIMP_HOME)
    {
        return; // Skip sending commands in all other states
    }

   //  if ((opmode == MOD_RUN || opmode == MOD_CHARGE) && can)
   //  {
        timer500++;
        if (timer500 == 5)
        {
            if (DCSetVal < 9.0f)  DCSetVal = 9.0f;
            if (DCSetVal > 16.0f) DCSetVal = 16.0f;
 
             int voltageValue = static_cast<int>((DCSetVal - 9.0f) * 146.0f);
             voltageValue &= 0x03FF;
 
             bytes[0] = voltageValue & 0xFF;
             bytes[1] = ((voltageValue >> 8) & 0x03) | 0x04;
 
             can->Send(TESLA_DCDC_CMD_ID, bytes, 3);
             timer500 = 0;
         }
    //  }
 }
