# DBC Decode Agent Workflow (MLB Charger)

## Purpose
Repeatable workflow to import one CAN message from DBC into `vw_mlb_charger` decode logic with:
- exact bit decoding
- `charger_status.LAD_*` mapping
- receiver comments
- `CM_` comment trace
- `VAL_` enum + enum-string `#define` (params style)

## Scope
Use for MLB charger messages such as `LAD_01`, `LAD_02`, etc.

## Step 1: Extract message definition from DBC
```bash
rg -n "BO_ 1380 LAD_01|SG_ LAD_|CM_ SG_ 1380|VAL_ 1380" documentation/*.dbc -S
```

Then inspect exact sections:
```bash
sed -n '2624,2665p' documentation/MLBevo_Gen2_MLBevo_HCAN_KMatrix_V8.18.01F_20190718_SE.dbc
sed -n '8108,8136p' documentation/MLBevo_Gen2_MLBevo_HCAN_KMatrix_V8.18.01F_20190718_SE.dbc
sed -n '50220,50234p' documentation/MLBevo_Gen2_MLBevo_HCAN_KMatrix_V8.18.01F_20190718_SE.dbc
```

## Step 2: Prepare signal table
For each `SG_` in the message, record:
- signal name
- start bit / length / endianness / signedness
- factor / offset
- physical unit
- receiver list (must be copied into decode-line comment)
- `CM_` text (must be copied into a code comment)
- `VAL_` entries (if present)

## Step 3: Add status fields in `ChargerStatus`
In `include/vw_mlb_charger.h`, add one field per signal:
- use exact DBC signal names where practical (`LAD_*`)
- choose type matching decoded value (raw or physical)
- keep legacy aliases if existing code still depends on older names

## Step 4: Add enums and enum-string defines
For each signal with `VAL_` entries:
- add `#define SIGNAL_ENUM "0=..., 1=..."`
- add corresponding `enum`
- keep naming consistent and uppercase like `param_prj.h`

## Step 5: Implement decode in `DecodeCAN()`
In `src/vw_mlb_charger.cpp`, under the message `case`:
- add sender comment (`Sender: ...`)
- decode every `SG_` exactly from DBC bit layout
- write to `charger_status.LAD_*`
- add receiver comment at end of each assignment line
- add `CM_` comment above each signal decode
- if needed, map decoded values into legacy fields used by existing logic

## Step 6: Validate usage
Check all downstream references:
```bash
rg -n "charger_status\\.(mode|ACvoltage|HVVoltage|current|temperature|LAD_)" src include -S
```
Confirm:
- no type regression (signed vs unsigned)
- no accidental behavior change in `TagParams()` / `emulateMLB()`

## Step 7: Build/test
Run project build and tests (if available) after each message migration.

## Per-signal checklist
- [ ] `SG_` extracted
- [ ] decode math matches `(factor, offset)`
- [ ] assignment to `charger_status.LAD_*`
- [ ] receiver comment added
- [ ] `CM_` comment added
- [ ] `VAL_` enum + `#define` added (when present)
- [ ] legacy alias updated (if needed)
