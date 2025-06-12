#ifndef PARAMS_H
#define PARAMS_H
class Param {
public:
    enum {
        BMS_ChargeLim,
        BMS_Vmin,
        BMS_Vmax,
        BMS_Tmin,
        BMS_Tmax,
        BMS_PackVoltage,
        BMS_DeltaCellVoltage,
        BMS_BalancingVoltage,
        BMS_BalancingActive,
        BMS_BalancingAnyActive,
        BMS_ActualCurrent,
        BMS_SOC,
        BMS_PackPower,
        BMS_State,
        BMS_DTC,
        BMS_TimeoutFault,
        BMS_MaxChargeCurrent,
        BMS_MaxDischargeCurrent,
        BMS_ShutdownRequest,
        BMS_ShutdownReady,
        BMS_ShutdownAcknowledge,
        BMS_DataValid,
        BMS_CONT_State,
        BMS_CONT_DTC,
        BMS_CONT_NegativeInput,
        BMS_CONT_PositiveInput,
        BMS_CONT_PrechargeInput,
        BMS_CONT_SupplyVoltageAvailable
    };
    static void SetFloat(int, float) {}
    static void SetInt(int, int) {}
};
#endif
