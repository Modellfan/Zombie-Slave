#ifndef PARAMS_H
#define PARAMS_H
class Param {
public:
    enum {
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
        BMS_SOH,
        BMS_AvgEnergyPerHour,
        BMS_RemainingTime,
        BMS_RemainingEnergy,
        BMS_State,
        BMS_BalancingStatus,
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
        BMS_CONT_SupplyVoltageAvailable,
        LVDU_vehicle_state,
        mlb_chr_PlugStatus,
        manual_standby_mode,
        LVDU_dcdc_input_power_off_threshold,
        LVDU_forceVCUsShutdown,
        HVCM_to_bms_hv_request,
        LVDU_hv_request_pending,
        HVCM_state,
        dcdc_input_power_off_confirmed,
        heater_off_confirmed,
        hv_comfort_functions_allowed
    };
    static void SetFloat(int, float) {}
    static void SetInt(int idx, int val) { values[idx] = val; }
    static int  GetInt(int idx) { return values[idx]; }

private:
    static int values[64];
};

// Storage for parameters will be defined in params.cpp
#endif
