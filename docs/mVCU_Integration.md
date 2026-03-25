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

## Zusätzlicher MLB-Parameter

Neu hinzugefügt wurde der Diagnose-/Messwert:

- `mlb_chr_LAD_Verlustleistung` (W)

Dieser Wert wird aus dem MLB-CAN-Decode übernommen und für die Berechnung der AC-Eingangsleistung verwendet.

## Hinweise

- Negative Werte werden vor dem Versand auf `0` begrenzt.
- Die Implementierung ist in einer separaten Device-Klasse `mVCUIntegration` umgesetzt.
