#include "teensyBMS.h"
#include "params.h"
#include <string.h>
#include <libopencm3/stm32/crc.h>

// CAN message ID for VCU -> BMS feedback
#define VCU_STATUS_MSG_ID 0x437

/*
Shutdown sequence (simplified)
------------------------------

 BMS        VCU       Contactors
  |--0x41F: request-->|
  |<--0x437: status---|
  |                   |--open HV
  |<--0x41F: ready----|
  |--0x41F: ack------>|
*/

void TeensyBMS::SetCanInterface(CanHardware* c) {
    if (c == nullptr) {
        can = nullptr;
        return;
    }

    can = c;
    can->RegisterUserMessage(0x41A); // MSG1: Voltage
    can->RegisterUserMessage(0x41B); // MSG2: Cell Temp
    can->RegisterUserMessage(0x41C); // MSG3: Limits/Fault
    can->RegisterUserMessage(0x41D); // MSG4: SOC/SOH
    can->RegisterUserMessage(0x41E); // MSG5: HMI

}

void TeensyBMS::DecodeCAN(int id, uint8_t* data) {
    // Store diagnostics for the unit tests/stubs (removed params)

    switch (id) {
        case 0x41A: parseMsg1(data); break;
        case 0x41B: parseMsg2(data); break;
        case 0x41C: parseMsg3(data); break;
        case 0x41D: parseMsg4(data); break;
        case 0x41E: parseMsg5(data); break;
    }
}

bool TeensyBMS::checkCrc(uint8_t* d) {
    uint32_t buf[2];
    memcpy(buf, d, 8);
    buf[1] &= 0x00FFFFFF; // clear CRC byte
    crc_reset();
    uint32_t crc = crc_calculate_block(buf, 2) & 0xFF;
    return crc == d[7];
}

void TeensyBMS::parseMsg1(uint8_t* d) {
    if (!checkCrc(d)) return;
    const uint16_t rawPackVoltage = static_cast<uint16_t>(d[0]) | (static_cast<uint16_t>(d[1]) << 8);
    packVoltage = rawPackVoltage / 10.0f;

    const uint16_t rawPackCurrent = static_cast<uint16_t>(d[2]) | (static_cast<uint16_t>(d[3]) << 8);
    actualCurrent = (static_cast<int32_t>(rawPackCurrent) - 5000) / 10.0f;
    vMin = d[4] / 50.0f;
    vMax = d[5] / 50.0f;
    timeoutCounter = BMS_TIMEOUT_TICKS;
}

void TeensyBMS::parseMsg2(uint8_t* d) {
    if (!checkCrc(d)) return;
    tMin = static_cast<float>(d[0]) - 40.0f;
    tMax = static_cast<float>(d[1]) - 40.0f;
    balancingVoltage = d[2] / 50.0f;
    deltaVoltage = d[3] / 100.0f;

    const uint16_t rawPackPower = static_cast<uint16_t>(d[4]) | (static_cast<uint16_t>(d[5]) << 8);
    const float packPowerKw = (static_cast<int32_t>(rawPackPower) - 30000) / 100.0f;
    packPower = packPowerKw * 1000.0f;
}

void TeensyBMS::parseMsg3(uint8_t* d) {
    if (!checkCrc(d)) return;
    maxDischargeCurrent = (d[0] | (d[1] << 8)) / 10.0f;
    maxChargeCurrent = (d[2] | (d[3] << 8)) / 10.0f;
    contactorState = d[4];
    dtc = d[5];
}

void TeensyBMS::parseMsg4(uint8_t* d) {
    if (!checkCrc(d)) return;
    soc = (d[0] | (d[1] << 8)) / 100.0f;
    // soh not used
    balancingActive = d[4] != 0;
    anyBalancing = balancingActive;
    state = d[5];
}

void TeensyBMS::parseMsg5(uint8_t* d) {
    if (!checkCrc(d)) return;
    // energy per hour and time to full currently unused
}

float TeensyBMS::MaxChargeCurrent() {
    return maxChargeCurrent;
}

void TeensyBMS::Task100Ms() {
    if (timeoutCounter > 0) {
        timeoutCounter--;
    }

    const bool timeout = timeoutCounter == 0;
    const bool fault = dtc != 0;
    const bool contactorFault = contactorDTC != 0;
    const bool bmsValid = !timeout && !fault && !contactorFault;

    if (timeout) {
        ErrorMessage::Post(ERR_BMS_TIMEOUT);
    }
    if (fault) {
        ErrorMessage::Post(ERR_BMS_FAULT);
    }
    if (contactorFault) {
        ErrorMessage::Post(ERR_BMS_CONTACTOR_FAULT);
    }

    Param::SetFloat(Param::BMS_Vmin, vMin);
    Param::SetFloat(Param::BMS_Vmax, vMax);
    Param::SetFloat(Param::BMS_Tmin, tMin);
    Param::SetFloat(Param::BMS_Tmax, tMax);
    Param::SetFloat(Param::BMS_PackVoltage, packVoltage);
    Param::SetFloat(Param::BMS_DeltaCellVoltage, deltaVoltage);
    Param::SetFloat(Param::BMS_BalancingVoltage, balancingVoltage);
    Param::SetInt(Param::BMS_BalancingActive, balancingActive);
    Param::SetInt(Param::BMS_BalancingAnyActive, anyBalancing);
    Param::SetFloat(Param::BMS_ActualCurrent, actualCurrent);
    Param::SetFloat(Param::BMS_SOC, soc);
    Param::SetFloat(Param::BMS_PackPower, packPower);
    Param::SetInt(Param::BMS_State, state);
    Param::SetInt(Param::BMS_DTC, dtc);
    Param::SetInt(Param::BMS_TimeoutFault, timeout);
    Param::SetFloat(Param::BMS_MaxChargeCurrent, maxChargeCurrent);
    Param::SetFloat(Param::BMS_MaxDischargeCurrent, maxDischargeCurrent);
    Param::SetInt(Param::BMS_ShutdownRequest, shutdownRequest);
    Param::SetInt(Param::BMS_ShutdownReady, shutdownReady);
    Param::SetInt(Param::BMS_ShutdownAcknowledge, shutdownAck);
    Param::SetInt(Param::BMS_DataValid, bmsValid);

    Param::SetInt(Param::BMS_CONT_State, contactorState);
    Param::SetInt(Param::BMS_CONT_DTC, contactorDTC);
    Param::SetInt(Param::BMS_CONT_NegativeInput, contactorNegativeInput);
    Param::SetInt(Param::BMS_CONT_PositiveInput, contactorPositiveInput);
    Param::SetInt(Param::BMS_CONT_PrechargeInput, contactorPrechargeInput);
    Param::SetInt(Param::BMS_CONT_SupplyVoltageAvailable, contactorSupplyAvailable);

    // Send VCU status back to the BMS every cycle (100ms)
    if (can) {
        uint8_t bytes[8] = {0};
        bytes[0] = static_cast<uint8_t>(Param::GetInt(Param::LVDU_vehicle_state));
        bytes[1] = static_cast<uint8_t>(Param::GetInt(Param::LVDU_forceVCUsShutdown));
        bytes[2] = static_cast<uint8_t>(Param::GetInt(Param::LVDU_connectHVcommand));
        bytes[3] = txCounter & 0x0F;
        // Calculate CRC over the first 7 bytes and place the result in the last
        // byte to match the CRC format used by the BMS messages. Bytes 4..6
        // remain unused and are transmitted as zero.
        uint32_t buf[2] = {0, 0};
        memcpy(buf, bytes, sizeof(buf));
        buf[1] &= 0x00FFFFFF;  // clear CRC byte
        crc_reset();
        uint32_t crc = crc_calculate_block(buf, 2) & 0xFF;
        bytes[7] = crc;
        can->Send(VCU_STATUS_MSG_ID, bytes, 8);
        txCounter = (txCounter + 1) & 0x0F;
    }
}

