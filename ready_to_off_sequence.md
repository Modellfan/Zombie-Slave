# READY -> Ignition OFF / Sleep Sequence (LVDU, Ready Relay, TeensyBMS)

This document describes what happens when the system is in `STATE_READY` and the
ignition input is turned OFF. It focuses on the ready relay behavior, the
separate HV request path, and the dedicated shutdown message that is sent before
the LVDU drops the VCU relay on the way to `STATE_SLEEP`.

Scope:
- Software paths in `LVDU` (`include/lvdu.h`)
- Ready relay (`DigIo::ready_out`)
- HV request (`HVCM_to_bms_hv_request`)
- Pre-sleep shutdown message (`LVDU_forceVCUsShutdown`)
- TeensyBMS VCU status feedback (`src/teensyBMS.cpp`)

## 1) Starting point: `STATE_READY`

While in `STATE_READY`:
- `hvManager.SetHVRequest(true)` is asserted.
- Outputs:
  - `DigIo::ready_out` is ON.
  - `DigIo::condition_out` is ON.
  - `DigIo::vcu_out` is ON.
- `HVCM_to_bms_hv_request` is set from the HV manager request (subject to load
  interlocks).

## 2) Ignition turns OFF

Input path:
- `UpdateInputs()` reads `ignition_in` and stores `ignitionOn`.
- `UpdateState()` sees `ignitionOn == false` in `STATE_READY`.

Transition:
- `STATE_READY` -> `STATE_CONDITIONING`

Side effects of the transition:
- `TransitionTo(STATE_CONDITIONING)` sets:
  - `diagnosePending = true`
  - `diagnoseTimer = LVDU_DIAGNOSE_DELAY_STEPS` (4000 ms)
- HV request is still ON because `STATE_CONDITIONING` is an HV-using state.

## 3) Ready relay behavior after ignition OFF

During `STATE_CONDITIONING`:
- `DigIo::ready_out` is:
  - ON while `diagnosePending == true`
  - OFF once the diagnose window expires

Diagnosis behavior:
- For up to `LVDU_DIAGNOSE_DELAY_STEPS`, the system expects `ready_safety_in`
  to remain asserted. If it drops, an error is posted.
- After the diagnose timer ends and cooldown passes, `ready_out` must be OFF
  and `ready_safety_in` must be OFF; otherwise errors are posted.

## 4) HV contactor manager (request vs feedback)

`hvManager` is the single place the LVDU stores the *request* to keep HV on.

Key points:
- `hvManager.SetHVRequest(true/false)` is controlled by the LVDU state machine.
- `hvManager.IsHVActive()` is *feedback only* and reflects BMS contactor state:
  - `IsHVActive()` is true only if:
    - `BMS_DataValid` is true, and
    - `BMS_CONT_State == 4` (closed)

So after ignition OFF:
- The request stays ON while in `STATE_CONDITIONING`.
- Actual HV feedback depends on the BMS contactor state.

## 5) What TeensyBMS sends to the BMS every 100 ms

Every 100 ms `TeensyBMS::Task100Ms()` sends the VCU status frame to the BMS:
- Byte 0: `LVDU_vehicle_state`
- Byte 1: `LVDU_forceVCUsShutdown`
- Byte 2: `HVCM_to_bms_hv_request`
- Byte 6: rolling counter
- Byte 7: CRC

Important distinction:
- `LVDU_forceVCUsShutdown` is the shutdown messaging bit that warns the BMS and
  downstream VCUs that the LVDU is about to drop the VCU relay before entering
  `STATE_SLEEP`.
- `HVCM_to_bms_hv_request` is only the HV request signal. It controls whether
  the BMS should keep HV active. It is **not** the shutdown message.

After ignition OFF:
- `STATE_CONDITIONING` keeps the HV request ON.
- Therefore `HVCM_to_bms_hv_request` remains 1 while conditioning is active,
  unless the HV-off interlock is explicitly allowing it to go low.

## 6) When does LVDU request HV OFF?

LVDU only drops the HV request when it transitions to a *non-HV* state:
- `STATE_STANDBY`
- `STATE_SLEEP`
- `STATE_ERROR` (explicitly requests HV off)

In the READY -> ignition OFF path:
- `READY` -> `CONDITIONING`
- Then, if `thermalTaskCompleted` and diagnosis is not pending:
  - `CONDITIONING` -> `STANDBY`
- On entering `STANDBY`, `hvManager.SetHVRequest(false)` is executed.

At that point, `HVCM_to_bms_hv_request` will go low unless HV loads are still
active and the HV-off interlock keeps it high.

## 7) How sleep shutdown messaging now works

Before the LVDU turns off `DigIo::vcu_out` and enters `STATE_SLEEP`, the sleep
call sites always transition into `STATE_FORCE_VCU_SHUTDOWN` first.

During `STATE_FORCE_VCU_SHUTDOWN`:
- `LVDU_forceVCUsShutdown = 1`
- `DigIo::vcu_out` stays ON
- `DigIo::condition_out` is OFF
- `DigIo::ready_out` is OFF

When the delay expires:
- the next 100 ms cycle transitions to `STATE_SLEEP`
- `LVDU_forceVCUsShutdown` returns to 0
- `DigIo::vcu_out` is turned OFF

When the configured delay is 0 ms:
- the system still enters `STATE_FORCE_VCU_SHUTDOWN`
- on the following 100 ms cycle it transitions to `STATE_SLEEP`

This same pre-sleep delay is used for:
- normal standby -> sleep shutdown
- error -> sleep shutdown
- forced low-voltage sleep shutdown

## 8) Summary sequence (high level)

1. `STATE_READY`, ignition ON:
   - `ready_out = ON`
   - `HVCM_to_bms_hv_request = 1`

2. Ignition OFF:
   - `STATE_READY` -> `STATE_CONDITIONING`
   - `ready_out = ON` during diagnose window
   - `HVCM_to_bms_hv_request = 1`

3. Diagnosis ends:
   - `ready_out = OFF`
   - still in `STATE_CONDITIONING`, HV request remains ON

4. Conditioning completes:
   - `STATE_CONDITIONING` -> `STATE_STANDBY`
   - `hvManager.SetHVRequest(false)`
   - `HVCM_to_bms_hv_request` goes low if no HV loads are active

5. Before any transition from `STATE_STANDBY` or `STATE_ERROR` to `STATE_SLEEP`
   completes:
   - `STATE_FORCE_VCU_SHUTDOWN` is entered
   - TeensyBMS sends VCU status with `LVDU_forceVCUsShutdown = 1`
   - `vcu_out` stays ON for the configured delay

6. After the delay expires:
   - `STATE_SLEEP` is entered
   - `vcu_out` turns OFF
   - `LVDU_forceVCUsShutdown` returns to 0

End state: `STATE_SLEEP` with the VCU relay off, after the shutdown warning was
held long enough to be transmitted.

Default setting:
- `LVDU_force_vcu_shutdown_delay = 1000 ms`
