# READY -> Ignition OFF Sequence (LVDU, Ready Relay, HV Manager, TeensyBMS)

This document describes what happens when the system is in `STATE_READY` and the
ignition input is turned OFF. It focuses on the ready relay behavior and the HV
contactor request/handshake path that goes through `LVDU` and `TeensyBMS`.

Scope:
- Software paths in `LVDU` (`include/lvdu.h`)
- Ready relay (`DigIo::ready_out`)
- HV contactor request (`LVDU_connectHVcommand`)
- TeensyBMS VCU status feedback (`src/teensyBMS.cpp`)

## 1) Starting point: `STATE_READY`

While in `STATE_READY`:
- `hvManager.SetHVRequest(true)` is asserted.
- Outputs:
  - `DigIo::ready_out` is ON.
  - `DigIo::condition_out` is ON.
  - `DigIo::vcu_out` is ON.
- `LVDU_connectHVcommand` is set from the HV manager request (subject to load
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

## 5) How LVDU requests HV from the BMS (TeensyBMS handshake)

Every 100 ms `TeensyBMS::Task100Ms()` sends the VCU status frame to the BMS:
- Byte 0: `LVDU_vehicle_state`
- Byte 1: `LVDU_forceVCUsShutdown`
- Byte 2: `LVDU_connectHVcommand`
- Byte 6: rolling counter
- Byte 7: CRC

The important piece is `LVDU_connectHVcommand`:
- When the LVDU wants HV on, it sets `LVDU_connectHVcommand = 1`.
- When LVDU wants HV off, it sets `LVDU_connectHVcommand = 0`.

After ignition OFF:
- `STATE_CONDITIONING` keeps the HV request ON.
- Therefore `LVDU_connectHVcommand` remains 1 while conditioning is active,
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

At that point, `LVDU_connectHVcommand` will go low unless HV loads are still
active and the HV-off interlock keeps it high.

## 7) Summary sequence (high level)

1. `STATE_READY`, ignition ON:
   - `ready_out = ON`
   - `LVDU_connectHVcommand = 1`

2. Ignition OFF:
   - `STATE_READY` -> `STATE_CONDITIONING`
   - `ready_out = ON` during diagnose window
   - `LVDU_connectHVcommand = 1`

3. Diagnosis ends:
   - `ready_out = OFF`
   - still in `STATE_CONDITIONING`, HV request remains ON

4. Conditioning completes:
   - `STATE_CONDITIONING` -> `STATE_STANDBY`
   - `hvManager.SetHVRequest(false)`
   - `LVDU_connectHVcommand` goes low if no HV loads are active

5. TeensyBMS sends VCU status with `LVDU_connectHVcommand = 0`
   - BMS opens contactors
   - LVDU sees `BMS_CONT_State` drop, `hvManager.IsHVActive()` becomes false

End state: `STATE_STANDBY` with ready relay OFF and HV contactors open.
