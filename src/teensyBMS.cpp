#include "teensyBMS.h"
#include "params.h"
#include <string.h>

void TeensyBMS::SetCanInterface(CanHardware* c) {
    can = c;
    can->RegisterUserMessage(0x41A); // 1050: Pack state
    can->RegisterUserMessage(0x41B); // 1051: Voltage info
    can->RegisterUserMessage(0x41C); // 1052: Temperatures
    can->RegisterUserMessage(0x41D); // 1053: Current + SOC
    can->RegisterUserMessage(0x41E); // 1054: Charge/discharge limits
    can->RegisterUserMessage(0x41F); // 1055: Shutdown handshake
    can->RegisterUserMessage(0x438); // 1080: Contactor manager state
}

void TeensyBMS::DecodeCAN(int id, uint8_t* data) {
    switch (id) {
        case 0x41A: parseMessage41A(data); break;
        case 0x41B: parseMessage41B(data); break;
        case 0x41C: parseMessage41C(data); break;
        case 0x41D: parseMessage41D(data); break;
        case 0x41E: parseMessage41E(data); break;
        case 0x41F: parseMessage41F(data); break;
        case 0x438: parseMessage438(data); break;
    }
}

void TeensyBMS::parseMessage41A(uint8_t* d) {
    state = d[0];
    dtc = d[1];
    balancingVoltage = (d[2] | (d[3] << 8)) * 0.001f;
    balancingActive = d[4] & 0x01;
    anyBalancing = (d[4] >> 1) & 0x01;
    timeoutCounter = BMS_TIMEOUT_TICKS;
}

void TeensyBMS::parseMessage41B(uint8_t* d) {
    vMin = (d[0] | (d[1] << 8)) * 0.001f;
    vMax = (d[2] | (d[3] << 8)) * 0.001f;
    packVoltage = (d[4] | (d[5] << 8)) * 0.01f;
    deltaVoltage = (d[6] | (d[7] << 8)) * 0.001f;
}

void TeensyBMS::parseMessage41C(uint8_t* d) {
    tMin = (int16_t)(d[0] | (d[1] << 8)) - 40;
    tMax = (int16_t)(d[2] | (d[3] << 8)) - 40;
}

void TeensyBMS::parseMessage41D(uint8_t* d) {
    actualCurrent = (int16_t)(d[0] | (d[1] << 8)) * 0.1f;
    soc = (d[2] | (d[3] << 8)) * 0.1f;
    packPower = d[4] | (d[5] << 8);
}

void TeensyBMS::parseMessage41E(uint8_t* d) {
    maxChargeCurrent = (d[0] | (d[1] << 8)) * 0.1f;
    maxDischargeCurrent = (d[2] | (d[3] << 8)) * 0.1f;
}

void TeensyBMS::parseMessage41F(uint8_t* d) {
    shutdownRequest = d[0];
    shutdownReady = d[1];
    shutdownAck = d[2];
}

void TeensyBMS::parseMessage438(uint8_t* d) {
    contactorState = d[0];
    contactorDTC = d[1];
    contactorNegativeInput = d[2] & 0x01;
    contactorPositiveInput = d[3] & 0x01;
    contactorPrechargeInput = d[4] & 0x01;
    contactorSupplyAvailable = d[5] & 0x01;
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
}
