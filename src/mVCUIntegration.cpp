#include "mVCUIntegration.h"
#include "params.h"

#define MVCU_CHARGE_POWER_STATUS_ID 0x438

void mVCUIntegration::SetCanInterface(CanHardware* c)
{
    can = c;
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
}
