# mVCU Integration

Diese Dokumentation beschreibt die aktuell implementierte CAN-Kommunikation zwischen Zombie-Slave und dem Main-Steuergerät (mVCU).

## Ziel

An das Main-Steuergerät werden zyklisch folgende Leistungswerte übertragen:

1. **Aktuelle Ladeleistung (als AC-Eingangsleistungs-Schätzwert)**
2. **Maximal mögliche Ladeleistung**

## CAN Nachricht (TX)

- **CAN-ID:** `0x438`
- **DLC:** `8`
- **Sendeintervall:** `100 ms`
- **Endianness:** Little Endian

### Signalbelegung

| Byte | Signal | Typ | Einheit | Quelle |
|---|---|---|---|---|
| 0..3 | `actualChargePower` | `uint32` | W | `(mlb_chr_LAD_IstSpannung_HV * mlb_chr_LAD_IstStrom_HV) + mlb_chr_LAD_Verlustleistung` |
| 4..7 | `maxChargePower` | `uint32` | W | `mlb_chr_HVLM_MaxLadeLeistung` |

## Heater Status Nachricht (TX)

- **CAN-ID:** `0x439`
- **DLC:** `8`
- **Sendeintervall:** `100 ms`
- **Sicherheitskonzept:** Byte 6 enthält einen 4-Bit Rolling Counter (0..15), Byte 7 enthält CRC8 (STM32 CRC Low-Byte über Byte 0..6)

| Byte | Signal | Typ | Bedeutung |
|---|---|---|---|
| 0 | `heater_active` | `uint8` | 0 = aus, 1 = aktiv |
| 1 | `heater_contactor_feedback_in` | `uint8` | 0 = offen, 1 = geschlossen |
| 2 | `heater_fault` | `uint8` | 0 = OK, 1 = Fehler |
| 3 | `heater_thermal_switch_in` | `uint8` | 0 = offen/Übertemp, 1 = geschlossen/OK |
| 4 | `heater_contactor_out` | `uint8` | 0 = aus, 1 = ein |
| 5 | `heater_can_contactor_request` | `uint8` | letzter gültiger mVCU-Heater-Anforderungsstatus |
| 6 | `counter` | `uint8` | low nibble 0..15 |
| 7 | `crc` | `uint8` | CRC8 |

## Heater Contactor Control Nachricht (RX)

- **CAN-ID:** `0x43A`
- **DLC:** `8` (Frames mit DLC < 8 werden verworfen)
- **Sicherheitskonzept:** CRC + Rolling Counter (nur bei gültigem CRC und gültigem Zähler wird die Anforderung übernommen)
- **Timeout:** Ohne gültige Nachricht für 500 ms wird die CAN-Anforderung auf 0 zurückgesetzt.

| Byte | Signal | Typ | Bedeutung |
|---|---|---|---|
| 0 | `heater_close_request` | `uint8` | Bit0: 1 = Heizungs-Schütz anfordern (wie analoges Flapsignal), 0 = aus |
| 1..5 | `reserved` | `uint8` | reserviert |
| 6 | `counter` | `uint8` | low nibble 0..15 |
| 7 | `crc` | `uint8` | CRC8 |

## Zusätzlicher MLB-Parameter

Neu hinzugefügt wurde der Diagnose-/Messwert:

- `mlb_chr_LAD_Verlustleistung` (W)

Dieser Wert wird aus dem MLB-CAN-Decode übernommen und für die Berechnung der AC-Eingangsleistung verwendet.

## Hinweise

- Negative Werte werden vor dem Versand auf `0` begrenzt.
- Die Implementierung ist in einer separaten Device-Klasse `mVCUIntegration` umgesetzt.
- Die CAN-Heater-Anforderung erweitert die bestehende Heater-Logik um einen zusätzlichen Triggerpfad (parallel zu manuellem Override und analogem Flapsignal).
