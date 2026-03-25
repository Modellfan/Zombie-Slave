#ifndef PARAMS_H
#define PARAMS_H
class Param {
public:
    enum {
        LVDU_12v_low_threshold,
        LVDU_hv_low_threshold,
        LVDU_force_vcu_shutdown_delay,
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
        LVDU_ignition_in,
        LVDU_ready_safety_in,
        LVDU_vehicle_state,
        LVDU_last_vehicle_state,
        LVDU_prev_prev_vehicle_state,
        LVDU_queued_vehicle_state,
        LVDU_prev_trigger_event,
        LVDU_prev_prev_trigger_event,
        LVDU_diagnose_pending,
        LVDU_12v_battery_voltage,
        LVDU_vcu_out,
        LVDU_condition_out,
        LVDU_ready_out,
        mlb_chr_PlugStatus,
        mlb_chr_HVLM_Stecker_Status,
        manual_standby_mode,
        charge_done_current,
        charge_done_delay,
        Charger_Plug_Override,
        LVDU_dcdc_input_power_off_threshold,
        LVDU_forceVCUsShutdown,
        HVCM_to_bms_hv_request,
        LVDU_hv_request_pending,
        HVCM_state,
        dcdc_input_power_off_confirmed,
        heater_off_confirmed,
        hv_comfort_functions_allowed
    };
    static void SetFloat(int idx, float val) { floatValues[idx] = val; }
    static float GetFloat(int idx) { return floatValues[idx]; }
    static void SetInt(int idx, int val) { values[idx] = val; }
    static int  GetInt(int idx) { return values[idx]; }

private:
    static int values[256];
    static float floatValues[256];
};

// Storage for parameters will be defined in params.cpp
#endif
