#include "mVCUIntegration.h"
#include "params.h"
#include <libopencm3/stm32/crc.h>
#include <string.h>

#define MVCU_CHARGE_POWER_STATUS_ID 0x438
#define MVCU_HEATER_STATUS_ID 0x439
#define MVCU_HEATER_CONTROL_ID 0x43A
#define MVCU_RX_TIMEOUT_TICKS_100MS 5

void mVCUIntegration::SetCanInterface(CanHardware* c)
{
    if (c == nullptr)
    {
        can = nullptr;
        heaterCanCloseRequest = false;
        haveSeenValidCounter = false;
        rxTimeoutTicks = 0;
        Param::SetInt(Param::heater_can_contactor_request, 0);
        return;
    }

    can = c;
    can->RegisterUserMessage(MVCU_HEATER_CONTROL_ID);
}

bool mVCUIntegration::CheckCrc(const uint8_t* data) const
{
    uint32_t buf[2] = {0, 0};
    memcpy(buf, data, 8);
    buf[1] &= 0x00FFFFFF; // clear CRC byte (byte 7)
    crc_reset();
    const uint32_t crc = crc_calculate_block(buf, 2) & 0xFFU;
    return crc == data[7];
}

bool mVCUIntegration::ValidateCounter(uint8_t counter)
{
    counter &= 0x0FU;

    if (!haveSeenValidCounter)
    {
        haveSeenValidCounter = true;
        lastRxCounter = counter;
        return true;
    }

    const uint8_t expectedCounter = static_cast<uint8_t>((lastRxCounter + 1U) & 0x0FU);
    if (counter != expectedCounter)
    {
        return false;
    }

    lastRxCounter = counter;
    return true;
}

void mVCUIntegration::DecodeCAN(int id, uint8_t* data, uint8_t dlc)
{
    if (id != MVCU_HEATER_CONTROL_ID || data == nullptr || dlc < 8)
    {
        return;
    }

    if (!CheckCrc(data))
    {
        return;
    }

    if (!ValidateCounter(data[6] & 0x0FU))
    {
        return;
    }

    heaterCanCloseRequest = (data[0] & 0x01U) != 0U;
    rxTimeoutTicks = MVCU_RX_TIMEOUT_TICKS_100MS;
    Param::SetInt(Param::heater_can_contactor_request, heaterCanCloseRequest ? 1 : 0);
}

void mVCUIntegration::Task100Ms()
{
    if (!can)
    {
        return;
    }

    int dcChargePowerW = Param::GetInt(Param::mlb_chr_LAD_IstSpannung_HV) * Param::GetInt(Param::mlb_chr_LAD_IstStrom_HV);
    if (dcChargePowerW < 0)
    {
        dcChargePowerW = 0;
    }

    int chargerLossPowerW = Param::GetInt(Param::mlb_chr_LAD_Verlustleistung);
    if (chargerLossPowerW < 0)
    {
        chargerLossPowerW = 0;
    }

    const int acInputPowerW = dcChargePowerW + chargerLossPowerW;

    int maxChargePowerW = Param::GetInt(Param::mlb_chr_HVLM_MaxLadeLeistung);
    if (maxChargePowerW < 0)
    {
        maxChargePowerW = 0;
    }

    uint8_t bytes[8] = {0};
    const uint32_t actualChargePower = static_cast<uint32_t>(acInputPowerW);
    const uint32_t maxChargePower = static_cast<uint32_t>(maxChargePowerW);

    bytes[0] = static_cast<uint8_t>(actualChargePower & 0xFFU);
    bytes[1] = static_cast<uint8_t>((actualChargePower >> 8) & 0xFFU);
    bytes[2] = static_cast<uint8_t>((actualChargePower >> 16) & 0xFFU);
    bytes[3] = static_cast<uint8_t>((actualChargePower >> 24) & 0xFFU);

    bytes[4] = static_cast<uint8_t>(maxChargePower & 0xFFU);
    bytes[5] = static_cast<uint8_t>((maxChargePower >> 8) & 0xFFU);
    bytes[6] = static_cast<uint8_t>((maxChargePower >> 16) & 0xFFU);
    bytes[7] = static_cast<uint8_t>((maxChargePower >> 24) & 0xFFU);

    can->Send(MVCU_CHARGE_POWER_STATUS_ID, bytes, 8);

    if (rxTimeoutTicks > 0)
    {
        rxTimeoutTicks--;
    }
    if (rxTimeoutTicks == 0)
    {
        heaterCanCloseRequest = false;
        Param::SetInt(Param::heater_can_contactor_request, 0);
    }

    uint8_t heaterStatusBytes[8] = {0};
    heaterStatusBytes[0] = static_cast<uint8_t>(Param::GetInt(Param::heater_active) ? 1 : 0);
    heaterStatusBytes[1] = static_cast<uint8_t>(Param::GetInt(Param::heater_contactor_feedback_in) ? 1 : 0);
    heaterStatusBytes[2] = static_cast<uint8_t>(Param::GetInt(Param::heater_fault) ? 1 : 0);
    heaterStatusBytes[3] = static_cast<uint8_t>(Param::GetInt(Param::heater_thermal_switch_in) ? 1 : 0);
    heaterStatusBytes[4] = static_cast<uint8_t>(Param::GetInt(Param::heater_contactor_out) ? 1 : 0);
    heaterStatusBytes[5] = static_cast<uint8_t>(Param::GetInt(Param::heater_can_contactor_request) ? 1 : 0);
    heaterStatusBytes[6] = txCounter & 0x0FU;

    uint32_t crcBuf[2] = {0, 0};
    memcpy(crcBuf, heaterStatusBytes, sizeof(crcBuf));
    crcBuf[1] &= 0x00FFFFFF; // clear CRC byte (byte 7)
    crc_reset();
    heaterStatusBytes[7] = static_cast<uint8_t>(crc_calculate_block(crcBuf, 2) & 0xFFU);

    can->Send(MVCU_HEATER_STATUS_ID, heaterStatusBytes, 8);
    txCounter = static_cast<uint8_t>((txCounter + 1U) & 0x0FU);
}
