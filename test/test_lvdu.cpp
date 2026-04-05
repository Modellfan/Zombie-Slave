#define private public
#include "lvdu.h"
#undef private

#include "anain.h"
#include "digio.h"
#include <cassert>

static void ResetIo()
{
    DigIo::ignition_in.Clear();
    DigIo::ready_safety_in.Clear();
    DigIo::ready_out.Clear();
    DigIo::condition_out.Clear();
    DigIo::vcu_out.Clear();
}

static void ConfigureCommonParams()
{
    Param::SetFloat(Param::LVDU_12v_low_threshold, 11.0f);
    Param::SetFloat(Param::LVDU_hv_low_threshold, 200.0f);
    Param::SetInt(Param::mlb_chr_HVLM_Stecker_Status, 1);
    Param::SetInt(Param::manual_standby_mode, 0);
    Param::SetInt(Param::BMS_DataValid, 0);
    Param::SetInt(Param::BMS_BalancingAnyActive, 0);
    Param::SetFloat(Param::BMS_PackVoltage, 0.0f);
    Param::SetInt(Param::dcdc_input_power_off_confirmed, 1);
    Param::SetInt(Param::heater_off_confirmed, 1);
    AnaIn::dc_power_supply.Set(3000.0f); // ~13.7 V after divider factor
}

static void AdvanceCycles(LVDU& lvdu, int cycles)
{
    for (int i = 0; i < cycles; ++i)
    {
        lvdu.Task100Ms();
    }
}

int main()
{
    {
        ResetIo();
        ConfigureCommonParams();
        Param::SetInt(Param::LVDU_force_vcu_shutdown_delay, 2000);

        LVDU lvdu;

        lvdu.Task100Ms(); // STATE_SLEEP -> STATE_STANDBY
        assert(Param::GetInt(Param::LVDU_vehicle_state) == STATE_STANDBY);
        assert(Param::GetInt(Param::LVDU_forceVCUsShutdown) == 0);

        AdvanceCycles(lvdu, LVDU_STANDBY_TIMEOUT_STEPS);
        assert(Param::GetInt(Param::LVDU_vehicle_state) == STATE_FORCE_VCU_SHUTDOWN);
        assert(Param::GetInt(Param::LVDU_forceVCUsShutdown) == 1);
        assert(DigIo::vcu_out.Get());
        assert(!DigIo::condition_out.Get());
        assert(!DigIo::ready_out.Get());

        AdvanceCycles(lvdu, 20);
        assert(Param::GetInt(Param::LVDU_vehicle_state) == STATE_FORCE_VCU_SHUTDOWN);
        assert(Param::GetInt(Param::LVDU_forceVCUsShutdown) == 1);
        assert(DigIo::vcu_out.Get());

        lvdu.Task100Ms();
        assert(Param::GetInt(Param::LVDU_vehicle_state) == STATE_SLEEP);
        assert(Param::GetInt(Param::LVDU_forceVCUsShutdown) == 0);
        assert(!DigIo::vcu_out.Get());
        assert(!DigIo::condition_out.Get());
        assert(!DigIo::ready_out.Get());
    }

    {
        ResetIo();
        ConfigureCommonParams();
        Param::SetInt(Param::LVDU_force_vcu_shutdown_delay, 1000);

        LVDU lvdu;
        lvdu.state = STATE_ERROR;
        lvdu.prevState = STATE_HV_DISCONNECTING;

        lvdu.Task100Ms();
        assert(Param::GetInt(Param::LVDU_vehicle_state) == STATE_FORCE_VCU_SHUTDOWN);
        assert(Param::GetInt(Param::LVDU_forceVCUsShutdown) == 1);
        assert(DigIo::vcu_out.Get());

        AdvanceCycles(lvdu, 10);
        assert(Param::GetInt(Param::LVDU_vehicle_state) == STATE_FORCE_VCU_SHUTDOWN);
        assert(Param::GetInt(Param::LVDU_forceVCUsShutdown) == 1);

        lvdu.Task100Ms();
        assert(Param::GetInt(Param::LVDU_vehicle_state) == STATE_SLEEP);
        assert(Param::GetInt(Param::LVDU_forceVCUsShutdown) == 0);
        assert(!DigIo::vcu_out.Get());
    }

    {
        ResetIo();
        ConfigureCommonParams();
        Param::SetInt(Param::LVDU_force_vcu_shutdown_delay, 0);

        LVDU lvdu;

        lvdu.Task100Ms(); // STATE_SLEEP -> STATE_STANDBY
        assert(Param::GetInt(Param::LVDU_vehicle_state) == STATE_STANDBY);

        AdvanceCycles(lvdu, LVDU_STANDBY_TIMEOUT_STEPS);
        assert(Param::GetInt(Param::LVDU_vehicle_state) == STATE_FORCE_VCU_SHUTDOWN);
        assert(Param::GetInt(Param::LVDU_forceVCUsShutdown) == 1);

        lvdu.Task100Ms();
        assert(Param::GetInt(Param::LVDU_vehicle_state) == STATE_SLEEP);
        assert(Param::GetInt(Param::LVDU_forceVCUsShutdown) == 0);
        assert(!DigIo::vcu_out.Get());
    }

    return 0;
}
