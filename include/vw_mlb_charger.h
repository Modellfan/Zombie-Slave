
/*
 * This file is part of the ZombieVerter project.
 *
 * Copyright (C) 2024 Mitch Elliott
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef vw_mlb_charger_h
#define vw_mlb_charger_h

#include "chargerhw.h"
#include "canhardware.h"
#include <stdint.h>
#include "my_fp.h"
#include "params.h"
#include "my_math.h"
#include "stm32_can.h"
#include "CANSPI.h"
#include "vag_utils.h"

#define LAD_ISTMODUS_ENUM "0=Standby, 1=AC_Netzladung, 3=DC_Netzladung, 4=PreCharge_aktiv, 5=Fehler, 7=Init"
enum LAD_IstModus_Enum
{
    LAD_ISTMODUS_STANDBY = 0,
    LAD_ISTMODUS_AC_NETZLADUNG = 1,
    LAD_ISTMODUS_DC_NETZLADUNG = 3,
    LAD_ISTMODUS_PRECHARGE_AKTIV = 4,
    LAD_ISTMODUS_FEHLER = 5,
    LAD_ISTMODUS_INIT = 7
};

#define LAD_STATUS_SPGFREIHEIT_ENUM "0=Init, 1=HV_Komponente_spannungsfrei, 2=HV_Komp_nicht_spannungsfrei, 3=Fehler_nicht_spannungsfrei"
enum LAD_Status_Spgfreiheit_Enum
{
    LAD_STATUS_SPGFREIHEIT_INIT = 0,
    LAD_STATUS_SPGFREIHEIT_HV_KOMPONENTE_SPANNUNGSFREI = 1,
    LAD_STATUS_SPGFREIHEIT_HV_KOMP_NICHT_SPANNUNGSFREI = 2,
    LAD_STATUS_SPGFREIHEIT_FEHLER_NICHT_SPANNUNGSFREI = 3
};

#define LAD_AC_ISTSPANNUNG_ENUM "510=Init, 511=Fehler"
enum LAD_AC_Istspannung_Enum
{
    LAD_AC_ISTSPANNUNG_INIT = 510,
    LAD_AC_ISTSPANNUNG_FEHLER = 511
};

#define LAD_ISTSPANNUNG_HV_ENUM "1022=Init, 1023=Fehler"
enum LAD_IstSpannung_HV_Enum
{
    LAD_ISTSPANNUNG_HV_INIT = 1022,
    LAD_ISTSPANNUNG_HV_FEHLER = 1023
};

#define LAD_ISTSTROM_HV_ENUM "1022=Init, 1023=Fehler"
enum LAD_IstStrom_HV_Enum
{
    LAD_ISTSTROM_HV_INIT = 1022,
    LAD_ISTSTROM_HV_FEHLER = 1023
};

#define LAD_TEMPERATUR_ENUM "254=Init, 255=Fehler"
enum LAD_Temperatur_Enum
{
    LAD_TEMPERATUR_INIT = 254,
    LAD_TEMPERATUR_FEHLER = 255
};

#define LAD_VERLUSTLEISTUNG_ENUM "254=Init, 255=Fehler"
enum LAD_Verlustleistung_Enum
{
    LAD_VERLUSTLEISTUNG_INIT = 254,
    LAD_VERLUSTLEISTUNG_FEHLER = 255
};

#define LAD_ABREGELUNG_TEMPERATUR_ENUM "0=keine_Begrenzung, 1=Abregelung_Temp"
enum LAD_Abregelung_Temperatur_Enum
{
    LAD_ABREGELUNG_TEMPERATUR_KEINE_BEGRENZUNG = 0,
    LAD_ABREGELUNG_TEMPERATUR_ABREGELUNG_TEMP = 1
};

#define LAD_ABREGELUNG_IU_EIN_AUS_ENUM "0=keine_Abregelung, 1=Abregelung_UI"
enum LAD_Abregelung_IU_Ein_Aus_Enum
{
    LAD_ABREGELUNG_IU_EIN_AUS_KEINE_ABREGELUNG = 0,
    LAD_ABREGELUNG_IU_EIN_AUS_ABREGELUNG_UI = 1
};

#define LAD_ABREGELUNG_BUCHSETEMP_ENUM "0=keine_Abregelung, 1=Abregelung_Buchsentemp"
enum LAD_Abregelung_BuchseTemp_Enum
{
    LAD_ABREGELUNG_BUCHSETEMP_KEINE_ABREGELUNG = 0,
    LAD_ABREGELUNG_BUCHSETEMP_ABREGELUNG_BUCHSENTEMP = 1
};

#define LAD_MAXLADLEISTUNG_HV_ENUM "510=Init, 511=Fehler"
enum LAD_MaxLadLeistung_HV_Enum
{
    LAD_MAXLADLEISTUNG_HV_INIT = 510,
    LAD_MAXLADLEISTUNG_HV_FEHLER = 511
};

#define LAD_PRX_STROMLIMIT_ENUM "0=13_Ampere, 1=20_Ampere, 2=32_Ampere, 3=63_Ampere, 6=Init, 7=Fehler"
enum LAD_PRX_Stromlimit_Enum
{
    LAD_PRX_STROMLIMIT_13_AMPERE = 0,
    LAD_PRX_STROMLIMIT_20_AMPERE = 1,
    LAD_PRX_STROMLIMIT_32_AMPERE = 2,
    LAD_PRX_STROMLIMIT_63_AMPERE = 3,
    LAD_PRX_STROMLIMIT_INIT = 6,
    LAD_PRX_STROMLIMIT_FEHLER = 7
};

#define LAD_CP_ERKENNUNG_ENUM "0=CP_nicht_erkannt, 1=CP_erkannt"
enum LAD_CP_Erkennung_Enum
{
    LAD_CP_ERKENNUNG_CP_NICHT_ERKANNT = 0,
    LAD_CP_ERKENNUNG_CP_ERKANNT = 1
};

#define LAD_STECKER_VERRIEGELT_ENUM "0=nicht_verriegelt, 1=verriegelt"
enum LAD_Stecker_Verriegelt_Enum
{
    LAD_STECKER_VERRIEGELT_NICHT_VERRIEGELT = 0,
    LAD_STECKER_VERRIEGELT_VERRIEGELT = 1
};

#define LAD_KUEHLBEDARF_ENUM "0=kein_Kuehlbedarf, 1=geringer_Kuehlbedarf, 2=mittlerer_Kuehlbedarf, 3=hoher_Kuehlbedarf"
enum LAD_Kuehlbedarf_Enum
{
    LAD_KUEHLBEDARF_KEIN_KUEHLBEDARF = 0,
    LAD_KUEHLBEDARF_GERINGER_KUEHLBEDARF = 1,
    LAD_KUEHLBEDARF_MITTLERER_KUEHLBEDARF = 2,
    LAD_KUEHLBEDARF_HOHER_KUEHLBEDARF = 3
};

#define LAD_MAXLADLEISTUNG_HV_OFFSET_ENUM "0=+0W, 1=+25W, 2=+50W, 3=+75W"
enum LAD_MaxLadLeistung_HV_Offset_Enum
{
    LAD_MAXLADLEISTUNG_HV_OFFSET_0W = 0,
    LAD_MAXLADLEISTUNG_HV_OFFSET_25W = 1,
    LAD_MAXLADLEISTUNG_HV_OFFSET_50W = 2,
    LAD_MAXLADLEISTUNG_HV_OFFSET_75W = 3
};

#define LAD_WARNZUSTAND_ENUM "0=keine_Warnung, 1=Warnung_aktiv"
enum LAD_Warnzustand_Enum
{
    LAD_WARNZUSTAND_KEINE_WARNUNG = 0,
    LAD_WARNZUSTAND_WARNUNG_AKTIV = 1
};

#define LAD_FEHLERZUSTAND_ENUM "0=kein_Fehler_aktiv, 1=Fehler_aktiv_Laden_beendet"
enum LAD_Fehlerzustand_Enum
{
    LAD_FEHLERZUSTAND_KEIN_FEHLER_AKTIV = 0,
    LAD_FEHLERZUSTAND_FEHLER_AKTIV_LADEN_BEENDET = 1
};

#define HVLM_HV_ABSTELLZEIT_ENUM "254=Init, 255=Fehler"
enum HVLM_HV_Abstellzeit_Enum
{
    HVLM_HV_ABSTELLZEIT_INIT = 254,
    HVLM_HV_ABSTELLZEIT_FEHLER = 255
};

#define HVLM_LADESYSTEMHINWEISE_ENUM "0=kein_Hinweis, 1=Ladesystem_defekt, 2=Ladesaeule_inkompatibel, 3=DC_Laden_nicht_moeglich"
enum HVLM_Ladesystemhinweise_Enum
{
    HVLM_LADESYSTEMHINWEISE_KEIN_HINWEIS = 0,
    HVLM_LADESYSTEMHINWEISE_LADESYSTEM_DEFEKT = 1,
    HVLM_LADESYSTEMHINWEISE_LADESAULE_INKOMPATIBEL = 2,
    HVLM_LADESYSTEMHINWEISE_DC_LADEN_NICHT_MOEGLICH = 3
};

#define HVLM_SCHLUESSEL_ANFRAGE_ENUM "0=Keine_Anfrage_Init, 1=Suchanfrage_LKL_1_aktiv, 2=Suchanfrage_LKL_2_aktiv, 3=Reserve"
enum HVLM_Schluessel_Anfrage_Enum
{
    HVLM_SCHLUESSEL_ANFRAGE_KEINE_ANFRAGE_INIT = 0,
    HVLM_SCHLUESSEL_ANFRAGE_SUCHANFRAGE_LKL_1_AKTIV = 1,
    HVLM_SCHLUESSEL_ANFRAGE_SUCHANFRAGE_LKL_2_AKTIV = 2,
    HVLM_SCHLUESSEL_ANFRAGE_RESERVE = 3
};

#define HVLM_ZUSTAND_LED_ENUM "0=Farbe_1_aus, 1=Farbe_2_weiss, 2=Farbe_3_gelb, 3=Farbe_4_gruen, 4=Farbe_5_rot, 5=Farbe_3_pulsierend, 6=Farbe_4_pulsierend, 7=Farbe_5_pulsierend, 8=Farbe_4_5_pulsierend, 9=Farbe_4_gruen_blinkend, 14=Init, 15=Fehler"
enum HVLM_Zustand_LED_Enum
{
    HVLM_ZUSTAND_LED_FARBE_1_AUS = 0,
    HVLM_ZUSTAND_LED_FARBE_2_WEISS = 1,
    HVLM_ZUSTAND_LED_FARBE_3_GELB = 2,
    HVLM_ZUSTAND_LED_FARBE_4_GRUEN = 3,
    HVLM_ZUSTAND_LED_FARBE_5_ROT = 4,
    HVLM_ZUSTAND_LED_FARBE_3_PULSIEREND = 5,
    HVLM_ZUSTAND_LED_FARBE_4_PULSIEREND = 6,
    HVLM_ZUSTAND_LED_FARBE_5_PULSIEREND = 7,
    HVLM_ZUSTAND_LED_FARBE_4_5_PULSIEREND = 8,
    HVLM_ZUSTAND_LED_FARBE_4_GRUEN_BLINKEND = 9,
    HVLM_ZUSTAND_LED_INIT = 14,
    HVLM_ZUSTAND_LED_FEHLER = 15
};

#define HVLM_MAXSTROM_NETZ_ENUM "126=Init, 127=Fehler"
enum HVLM_MaxStrom_Netz_Enum
{
    HVLM_MAXSTROM_NETZ_INIT = 126,
    HVLM_MAXSTROM_NETZ_FEHLER = 127
};

#define HVLM_LG_SOLLMODUS_ENUM "0=Standby, 1=Netzladung"
enum HVLM_LG_Sollmodus_Enum
{
    HVLM_LG_SOLLMODUS_STANDBY = 0,
    HVLM_LG_SOLLMODUS_NETZLADUNG = 1
};

#define HVLM_FREIGABETANKDECKEL_ENUM "0=keine_Freigabe, 1=Freigabe, 2=Init, 3=Fehler"
enum HVLM_FreigabeTankdeckel_Enum
{
    HVLM_FREIGABETANKDECKEL_KEINE_FREIGABE = 0,
    HVLM_FREIGABETANKDECKEL_FREIGABE = 1,
    HVLM_FREIGABETANKDECKEL_INIT = 2,
    HVLM_FREIGABETANKDECKEL_FEHLER = 3
};

#define HVLM_STECKER_VERRIEGELN_ENUM "0=Stecker_entriegeln, 1=Stecker_verriegeln, 2=Init, 3=keine_Anforderung"
enum HVLM_Stecker_Verriegeln_Enum
{
    HVLM_STECKER_VERRIEGELN_STECKER_ENTRIEGELN = 0,
    HVLM_STECKER_VERRIEGELN_STECKER_VERRIEGELN = 1,
    HVLM_STECKER_VERRIEGELN_INIT = 2,
    HVLM_STECKER_VERRIEGELN_KEINE_ANFORDERUNG = 3
};

#define HVLM_START_SPANNUNGSMESSUNG_DCLS_ENUM "0=inaktiv, 1=DCLS_mit_Diode_Start_Messung, 2=DCLS_ohne_Diode_Start_Messung, 3=reserve"
enum HVLM_Start_Spannungsmessung_DCLS_Enum
{
    HVLM_START_SPANNUNGSMESSUNG_DCLS_INAKTIV = 0,
    HVLM_START_SPANNUNGSMESSUNG_DCLS_MIT_DIODE = 1,
    HVLM_START_SPANNUNGSMESSUNG_DCLS_OHNE_DIODE = 2,
    HVLM_START_SPANNUNGSMESSUNG_DCLS_RESERVE = 3
};

#define HVLM_FREIGABEKLIMATISIERUNG_ENUM "0=keine_Freigabe, 1=Freigabe, 2=Init, 3=Fehler"
enum HVLM_FreigabeKlimatisierung_Enum
{
    HVLM_FREIGABEKLIMATISIERUNG_KEINE_FREIGABE = 0,
    HVLM_FREIGABEKLIMATISIERUNG_FREIGABE = 1,
    HVLM_FREIGABEKLIMATISIERUNG_INIT = 2,
    HVLM_FREIGABEKLIMATISIERUNG_FEHLER = 3
};

#define HVLM_LADETEXTE_ENUM "0=Keine_Anzeige, 1=AC_Laden_nicht_moeglich, 2=DC_Laden_nicht_moeglich, 3=AC_und_DC_Laden_nicht_moeglich"
enum HVLM_Ladetexte_Enum
{
    HVLM_LADETEXTE_KEINE_ANZEIGE = 0,
    HVLM_LADETEXTE_AC_LADEN_NICHT_MOEGLICH = 1,
    HVLM_LADETEXTE_DC_LADEN_NICHT_MOEGLICH = 2,
    HVLM_LADETEXTE_AC_UND_DC_LADEN_NICHT_MOEGLICH = 3
};

#define HVLM_ISOMESSUNG_ANF_ENUM "0=Laden_mit_Isomessung, 1=Laden_ohne_Isomessung"
enum HVLM_IsoMessung_Anf_Enum
{
    HVLM_ISOMESSUNG_ANF_LADEN_MIT_ISOMESSUNG = 0,
    HVLM_ISOMESSUNG_ANF_LADEN_OHNE_ISOMESSUNG = 1
};

#define HVLM_ISTSPANNUNG_HV_ENUM "1022=Init, 1023=Fehler"
enum HVLM_IstSpannung_HV_Enum
{
    HVLM_ISTSPANNUNG_HV_INIT = 1022,
    HVLM_ISTSPANNUNG_HV_FEHLER = 1023
};

#define HVLM_STH_BETRIEBSMODUS_ENUM "0=Keine_Klimatisierung, 1=Heizen, 2=Abtauen, 6=Init, 7=Fehler"
enum HVLM_STH_Betriebsmodus_Enum
{
    HVLM_STH_BETRIEBSMODUS_KEINE_KLIMATISIERUNG = 0,
    HVLM_STH_BETRIEBSMODUS_HEIZEN = 1,
    HVLM_STH_BETRIEBSMODUS_ABTAUEN = 2,
    HVLM_STH_BETRIEBSMODUS_INIT = 6,
    HVLM_STH_BETRIEBSMODUS_FEHLER = 7
};

#define HVLM_STANDKLIMA_TIMER_STATUS_ENUM "0=Standklimatimer_nicht_aktiv, 1=Standklimatimer_aktiv"
enum HVLM_Standklima_Timer_Status_Enum
{
    HVLM_STANDKLIMA_TIMER_STATUS_NICHT_AKTIV = 0,
    HVLM_STANDKLIMA_TIMER_STATUS_AKTIV = 1
};

#define HVLM_HVEM_MAXLEISTUNG_ENUM "510=Init, 511=Fehler"
enum HVLM_HVEM_MaxLeistung_Enum
{
    HVLM_HVEM_MAXLEISTUNG_INIT = 510,
    HVLM_HVEM_MAXLEISTUNG_FEHLER = 511
};

#define HVLM_STATUS_NETZ_ENUM "0=Fzg_nicht_am_Netz, 1=Fzg_ist_am_Stromnetz"
enum HVLM_Status_Netz_Enum
{
    HVLM_STATUS_NETZ_FZG_NICHT_AM_NETZ = 0,
    HVLM_STATUS_NETZ_FZG_IST_AM_STROMNETZ = 1
};

#define HVLM_ANF_LADESCREEN_ENUM "0=Ladescreen_off, 1=Ladescreen_on"
enum HVLM_Anf_Ladescreen_Enum
{
    HVLM_ANF_LADESCREEN_OFF = 0,
    HVLM_ANF_LADESCREEN_ON = 1
};

#define HVLM_LADEART_ENUM "0=keine_Auswahl, 1=AC_Laden, 2=DC_Laden, 3=Konditionierung"
enum HVLM_Ladeart_Enum
{
    HVLM_LADEART_KEINE_AUSWAHL = 0,
    HVLM_LADEART_AC_LADEN = 1,
    HVLM_LADEART_DC_LADEN = 2,
    HVLM_LADEART_KONDITIONIERUNG = 3
};

#define HVLM_VK_STH_EINSATZ_ENUM "0=Verbrennerstandheizung_verbieten, 1=Automatik, 2=Verbrennerstandheizung_erzwingen, 3=reserviert, 4=reserviert, 5=reserviert, 6=Init, 7=Fehler"
enum HVLM_VK_STH_Einsatz_Enum
{
    HVLM_VK_STH_EINSATZ_VERBIETEN = 0,
    HVLM_VK_STH_EINSATZ_AUTOMATIK = 1,
    HVLM_VK_STH_EINSATZ_ERZWINGEN = 2,
    HVLM_VK_STH_EINSATZ_RESERVIERT_3 = 3,
    HVLM_VK_STH_EINSATZ_RESERVIERT_4 = 4,
    HVLM_VK_STH_EINSATZ_RESERVIERT_5 = 5,
    HVLM_VK_STH_EINSATZ_INIT = 6,
    HVLM_VK_STH_EINSATZ_FEHLER = 7
};

#define HVLM_VK_MODUS_ENUM "0=keine_Vorkonditionierung, 1=Sofort_Konditionierung, 2=zeitversetzte_Konditionierung, 3=Fehler"
enum HVLM_VK_Modus_Enum
{
    HVLM_VK_MODUS_KEINE_VORKONDITIONIERUNG = 0,
    HVLM_VK_MODUS_SOFORT_KONDITIONIERUNG = 1,
    HVLM_VK_MODUS_ZEITVERSETZTE_KONDITIONIERUNG = 2,
    HVLM_VK_MODUS_FEHLER = 3
};

#define HVLM_ISTMODUS_02_ENUM "0=inaktiv, 1=aktiv, 2=Init, 3=Fehler"
enum HVLM_IstModus_02_Enum
{
    HVLM_ISTMODUS_02_INAKTIV = 0,
    HVLM_ISTMODUS_02_AKTIV = 1,
    HVLM_ISTMODUS_02_INIT = 2,
    HVLM_ISTMODUS_02_FEHLER = 3
};

#define HVLM_HV_ANF_ENUM "0=keine_Anforderung, 1=Laden_angefordert, 2=Batteriekonditionierung_angefordert, 3=Standklimatisierung_angefordert"
enum HVLM_HV_Anf_Enum
{
    HVLM_HV_ANF_KEINE_ANFORDERUNG = 0,
    HVLM_HV_ANF_LADEN_ANGEFORDERT = 1,
    HVLM_HV_ANF_BATTERIEKONDITIONIERUNG_ANGEFORDERT = 2,
    HVLM_HV_ANF_STANDKLIMATISIERUNG_ANGEFORDERT = 3
};

#define HVLM_FEHLERSTATUS_ENUM "0=Komponente_IO, 1=Eingeschr_KompFkt_DC_Lad_NIO, 2=Eingeschr_KompFkt_AC_Lad_NIO, 3=Eingeschr_KompFkt_Interlock, 4=Eingeschr_KompFkt_reserve, 5=Eingeschr_KompFkt_reserve, 6=Keine_Komponentenfunktion, 7=Init"
enum HVLM_Fehlerstatus_Enum
{
    HVLM_FEHLERSTATUS_KOMPONENTE_IO = 0,
    HVLM_FEHLERSTATUS_DC_LAD_NIO = 1,
    HVLM_FEHLERSTATUS_AC_LAD_NIO = 2,
    HVLM_FEHLERSTATUS_INTERLOCK = 3,
    HVLM_FEHLERSTATUS_RESERVE_4 = 4,
    HVLM_FEHLERSTATUS_RESERVE_5 = 5,
    HVLM_FEHLERSTATUS_KEINE_KOMPONENTENFUNKTION = 6,
    HVLM_FEHLERSTATUS_INIT = 7
};

#define HVLM_ANFORDERUNG_HMS_ENUM "0=keine_Anforderung, 1=halten, 2=parken, 3=halten_Standby, 4=anfahren, 5=Loesen_ueber_Rampe"
enum HVLM_Anforderung_HMS_Enum
{
    HVLM_ANFORDERUNG_HMS_KEINE_ANFORDERUNG = 0,
    HVLM_ANFORDERUNG_HMS_HALTEN = 1,
    HVLM_ANFORDERUNG_HMS_PARKEN = 2,
    HVLM_ANFORDERUNG_HMS_HALTEN_STANDBY = 3,
    HVLM_ANFORDERUNG_HMS_ANFAHREN = 4,
    HVLM_ANFORDERUNG_HMS_LOESEN_UEBER_RAMPE = 5
};

#define HVLM_PARKEN_BEIBEHALTEN_HMS_ENUM "0=Parken_nicht_beibehalten, 1=Parken_beibehalten, 2=Init, 3=Fehler"
enum HVLM_Parken_beibehalten_HMS_Enum
{
    HVLM_PARKEN_BEIBEHALTEN_HMS_NICHT_BEIBEHALTEN = 0,
    HVLM_PARKEN_BEIBEHALTEN_HMS_BEIBEHALTEN = 1,
    HVLM_PARKEN_BEIBEHALTEN_HMS_INIT = 2,
    HVLM_PARKEN_BEIBEHALTEN_HMS_FEHLER = 3
};

#define HVLM_AWC_SOLLMODUS_ENUM "0=init, 1=Inaktiv, 2=Standby, 3=AWCLaden, 4=Notabschaltung, 5=Reserve, 6=Reserve, 7=Fehler"
enum HVLM_AWC_Sollmodus_Enum
{
    HVLM_AWC_SOLLMODUS_INIT = 0,
    HVLM_AWC_SOLLMODUS_INAKTIV = 1,
    HVLM_AWC_SOLLMODUS_STANDBY = 2,
    HVLM_AWC_SOLLMODUS_AWCLADEN = 3,
    HVLM_AWC_SOLLMODUS_NOTABSCHALTUNG = 4,
    HVLM_AWC_SOLLMODUS_RESERVE_5 = 5,
    HVLM_AWC_SOLLMODUS_RESERVE_6 = 6,
    HVLM_AWC_SOLLMODUS_FEHLER = 7
};

#define HVLM_STECKER_STATUS_ENUM "0=Init, 1=kein_Stecker_gesteckt, 2=Stecker_erkannt_nicht_verriegelt, 3=Stecker_erkannt_und_verriegelt"
enum HVLM_Stecker_Status_Enum
{
    HVLM_STECKER_STATUS_INIT = 0,
    HVLM_STECKER_STATUS_KEIN_STECKER = 1,
    HVLM_STECKER_STATUS_ERKANNT_NICHT_VERRIEGELT = 2,
    HVLM_STECKER_STATUS_ERKANNT_VERRIEGELT = 3
};

#define HVLM_LADEANFORDERUNG_ENUM "0=keine_Anforderung, 1=AC_Laden, 2=DC_Laden, 3=Nachladen_12V, 4=AC_Laden_AWC, 5=Reserve, 6=Init, 7=Fehler"
enum HVLM_LadeAnforderung_Enum
{
    HVLM_LADEANFORDERUNG_KEINE_ANFORDERUNG = 0,
    HVLM_LADEANFORDERUNG_AC_LADEN = 1,
    HVLM_LADEANFORDERUNG_DC_LADEN = 2,
    HVLM_LADEANFORDERUNG_NACHLADEN_12V = 3,
    HVLM_LADEANFORDERUNG_AC_LADEN_AWC = 4,
    HVLM_LADEANFORDERUNG_RESERVE = 5,
    HVLM_LADEANFORDERUNG_INIT = 6,
    HVLM_LADEANFORDERUNG_FEHLER = 7
};

#define HVLM_MAXBATLADESTROMHV_ENUM "254=Init, 255=Fehler"
enum HVLM_MaxBatLadestromHV_Enum
{
    HVLM_MAXBATLADESTROMHV_INIT = 254,
    HVLM_MAXBATLADESTROMHV_FEHLER = 255
};

#define HVLM_MAXLADELEISTUNG_ENUM "1022=Init, 1023=Fehler"
enum HVLM_MaxLadeLeistung_Enum
{
    HVLM_MAXLADELEISTUNG_INIT = 1022,
    HVLM_MAXLADELEISTUNG_FEHLER = 1023
};

#define HVLM_MAXSPANNUNG_DCLS_ENUM "1022=Init, 1023=Fehler"
enum HVLM_MaxSpannung_DCLS_Enum
{
    HVLM_MAXSPANNUNG_DCLS_INIT = 1022,
    HVLM_MAXSPANNUNG_DCLS_FEHLER = 1023
};

#define HVLM_ISTSTROM_DCLS_ENUM "510=Init, 511=Fehler"
enum HVLM_IstStrom_DCLS_Enum
{
    HVLM_ISTSTROM_DCLS_INIT = 510,
    HVLM_ISTSTROM_DCLS_FEHLER = 511
};

#define HVLM_MAXSTROM_DCLS_ENUM "510=Init, 511=Fehler"
enum HVLM_MaxStrom_DCLS_Enum
{
    HVLM_MAXSTROM_DCLS_INIT = 510,
    HVLM_MAXSTROM_DCLS_FEHLER = 511
};

#define HVLM_MINSPANNUNG_DCLS_ENUM "510=Init, 511=Fehler"
enum HVLM_MinSpannung_DCLS_Enum
{
    HVLM_MINSPANNUNG_DCLS_INIT = 510,
    HVLM_MINSPANNUNG_DCLS_FEHLER = 511
};

#define HVLM_MINSTROM_DCLS_ENUM "30=Init, 31=Fehler"
enum HVLM_MinStrom_DCLS_Enum
{
    HVLM_MINSTROM_DCLS_INIT = 30,
    HVLM_MINSTROM_DCLS_FEHLER = 31
};

#define HVLM_FUNKTION_DAUER_BATTKOND_ENUM "0=inaktiv, 1=aktiv"
enum HVLM_Funktion_Dauer_BattKond_Enum
{
    HVLM_FUNKTION_DAUER_BATTKOND_INAKTIV = 0,
    HVLM_FUNKTION_DAUER_BATTKOND_AKTIV = 1
};

#define HVLM_BATTKOND_ANF_ENUM "0=keine_Anforderung, 1=Anforderung_Heizstufe_1, 2=Anforderung_Heizstufe_2, 6=Init, 7=Fehler"
enum HVLM_BattKond_Anf_Enum
{
    HVLM_BATTKOND_ANF_KEINE_ANFORDERUNG = 0,
    HVLM_BATTKOND_ANF_HEIZSTUFE_1 = 1,
    HVLM_BATTKOND_ANF_HEIZSTUFE_2 = 2,
    HVLM_BATTKOND_ANF_INIT = 6,
    HVLM_BATTKOND_ANF_FEHLER = 7
};

#define HVLM_DAUER_KLIMA_02_ENUM "61=default, 62=Init, 63=Fehler"
enum HVLM_Dauer_Klima_02_Enum
{
    HVLM_DAUER_KLIMA_02_DEFAULT = 61,
    HVLM_DAUER_KLIMA_02_INIT = 62,
    HVLM_DAUER_KLIMA_02_FEHLER = 63
};

#define HVLM_AWC_SOLLSTROM_ENUM "510=Init, 511=Fehler"
enum HVLM_AWC_Sollstrom_Enum
{
    HVLM_AWC_SOLLSTROM_INIT = 510,
    HVLM_AWC_SOLLSTROM_FEHLER = 511
};

#define HVLM_RTMWARNLADEVERBINDUNG_ENUM "0=kein_Fehler, 1=Fehler_Stufe_1, 2=Fehler_Stufe_2, 3=Fehler_Stufe_3, 4=reserviert, 5=reserviert, 6=reserviert, 7=reserviert"
enum HVLM_RtmWarnLadeverbindung_Enum
{
    HVLM_RTMWARNLADEVERBINDUNG_KEIN_FEHLER = 0,
    HVLM_RTMWARNLADEVERBINDUNG_FEHLER_STUFE_1 = 1,
    HVLM_RTMWARNLADEVERBINDUNG_FEHLER_STUFE_2 = 2,
    HVLM_RTMWARNLADEVERBINDUNG_FEHLER_STUFE_3 = 3
};

#define HVLM_RTMWARNLADESYSTEM_ENUM "0=kein_Fehler, 1=Fehler_Stufe_1, 2=Fehler_Stufe_2, 3=Fehler_Stufe_3, 4=reserviert, 5=reserviert, 6=reserviert, 7=reserviert"
enum HVLM_RtmWarnLadesystem_Enum
{
    HVLM_RTMWARNLADESYSTEM_KEIN_FEHLER = 0,
    HVLM_RTMWARNLADESYSTEM_FEHLER_STUFE_1 = 1,
    HVLM_RTMWARNLADESYSTEM_FEHLER_STUFE_2 = 2,
    HVLM_RTMWARNLADESYSTEM_FEHLER_STUFE_3 = 3
};

#define HVLM_RTMWARNLADESTATUS_ENUM "0=kein_Fehler, 1=Fehler_Stufe_1, 2=Fehler_Stufe_2, 3=Fehler_Stufe_3, 4=reserviert, 5=reserviert, 6=reserviert, 7=reserviert"
enum HVLM_RtmWarnLadestatus_Enum
{
    HVLM_RTMWARNLADESTATUS_KEIN_FEHLER = 0,
    HVLM_RTMWARNLADESTATUS_FEHLER_STUFE_1 = 1,
    HVLM_RTMWARNLADESTATUS_FEHLER_STUFE_2 = 2,
    HVLM_RTMWARNLADESTATUS_FEHLER_STUFE_3 = 3
};

#define HVLM_RTMWARNLADEKOMMUNIKATION_ENUM "0=kein_Fehler, 1=Fehler_Stufe_1, 2=Fehler_Stufe_2, 3=Fehler_Stufe_3, 4=reserviert, 5=reserviert, 6=reserviert, 7=reserviert"
enum HVLM_RtmWarnLadeKommunikation_Enum
{
    HVLM_RTMWARNLADEKOMMUNIKATION_KEIN_FEHLER = 0,
    HVLM_RTMWARNLADEKOMMUNIKATION_FEHLER_STUFE_1 = 1,
    HVLM_RTMWARNLADEKOMMUNIKATION_FEHLER_STUFE_2 = 2,
    HVLM_RTMWARNLADEKOMMUNIKATION_FEHLER_STUFE_3 = 3
};

#define HVLM_LADEANZEIGE_ANF_ENUM "0=Licht_Aus, 1=Licht_An"
enum HVLM_Ladeanzeige_Anf_Enum
{
    HVLM_LADEANZEIGE_ANF_LICHT_AUS = 0,
    HVLM_LADEANZEIGE_ANF_LICHT_AN = 1
};

#define HVLM_LADEANZEIGE_STATUS_ENUM "0=Funktion_Aus, 1=Funktion_Ein_ohne_SOC, 2=Funktion_Ein_mit_SOC, 3=Reserve"
enum HVLM_Ladeanzeige_Status_Enum
{
    HVLM_LADEANZEIGE_STATUS_FUNKTION_AUS = 0,
    HVLM_LADEANZEIGE_STATUS_FUNKTION_EIN_OHNE_SOC = 1,
    HVLM_LADEANZEIGE_STATUS_FUNKTION_EIN_MIT_SOC = 2,
    HVLM_LADEANZEIGE_STATUS_RESERVE = 3
};

#define HVLM_LADEANZEIGE_INTENS_HECK_ENUM "126=Init, 127=Fehler"
enum HVLM_Ladeanzeige_Intens_Heck_Enum
{
    HVLM_LADEANZEIGE_INTENS_HECK_INIT = 126,
    HVLM_LADEANZEIGE_INTENS_HECK_FEHLER = 127
};

#define HVLM_LADEANZEIGE_INTENS_FRONT_ENUM "126=Init, 127=Fehler"
enum HVLM_Ladeanzeige_Intens_Front_Enum
{
    HVLM_LADEANZEIGE_INTENS_FRONT_INIT = 126,
    HVLM_LADEANZEIGE_INTENS_FRONT_FEHLER = 127
};

#define HVLM_HVLB_SOLLSPANNUNG_HVLS_ENUM "4094=Init, 4095=Fehler"
enum HVLM_HVLB_SollSpannung_HVLS_Enum
{
    HVLM_HVLB_SOLLSPANNUNG_HVLS_INIT = 4094,
    HVLM_HVLB_SOLLSPANNUNG_HVLS_FEHLER = 4095
};

#define HVLM_HVLB_SOLLMODUS_ENUM "0=Standby, 1=Laden_DC_ohneBoost, 2=Laden_DC_mitBoost, 3=Laden_AC, 4=Notabschaltung, 5=Laden_DC_ohneBoost_ohneVorladung, 6=Laden_DC_mitBoost_ohneVorladung, 7=Init"
enum HVLM_HVLB_SollModus_Enum
{
    HVLM_HVLB_SOLLMODUS_STANDBY = 0,
    HVLM_HVLB_SOLLMODUS_LADEN_DC_OHNEBOOST = 1,
    HVLM_HVLB_SOLLMODUS_LADEN_DC_MITBOOST = 2,
    HVLM_HVLB_SOLLMODUS_LADEN_AC = 3,
    HVLM_HVLB_SOLLMODUS_NOTABSCHALTUNG = 4,
    HVLM_HVLB_SOLLMODUS_LADEN_DC_OHNEBOOST_OHNEVORLADUNG = 5,
    HVLM_HVLB_SOLLMODUS_LADEN_DC_MITBOOST_OHNEVORLADUNG = 6,
    HVLM_HVLB_SOLLMODUS_INIT = 7
};

#define HVLM_PLANANFR_LEISTUNG_ENUM "4094=Init, 4095=Fehler"
enum HVLM_PlanAnfr_Leistung_Enum
{
    HVLM_PLANANFR_LEISTUNG_INIT = 4094,
    HVLM_PLANANFR_LEISTUNG_FEHLER = 4095
};

#define HVLM_PLANANFR_ZAEHLER_ENUM "14=Init, 15=Fehler"
enum HVLM_PlanAnfr_Zaehler_Enum
{
    HVLM_PLANANFR_ZAEHLER_INIT = 14,
    HVLM_PLANANFR_ZAEHLER_FEHLER = 15
};

#define HVLM_PLANANFR_DAUER_ENUM "1022=Init, 1023=Fehler"
enum HVLM_PlanAnfr_Dauer_Enum
{
    HVLM_PLANANFR_DAUER_INIT = 1022,
    HVLM_PLANANFR_DAUER_FEHLER = 1023
};

#define HVLM_PLANANFR_LADEART_ENUM "0=keine_Ladeart, 1=AC_Laden, 2=DC_Laden_ohne_Boost, 3=DC_Laden_mit_Boost, 4=AWC_Laden, 5=Reserve, 6=Init, 7=Fehler"
enum HVLM_PlanAnfr_Ladeart_Enum
{
    HVLM_PLANANFR_LADEART_KEINE_LADEART = 0,
    HVLM_PLANANFR_LADEART_AC_LADEN = 1,
    HVLM_PLANANFR_LADEART_DC_LADEN_OHNE_BOOST = 2,
    HVLM_PLANANFR_LADEART_DC_LADEN_MIT_BOOST = 3,
    HVLM_PLANANFR_LADEART_AWC_LADEN = 4,
    HVLM_PLANANFR_LADEART_RESERVE = 5,
    HVLM_PLANANFR_LADEART_INIT = 6,
    HVLM_PLANANFR_LADEART_FEHLER = 7
};

#define HVLM_ENERGIEANFR_SOCSTART_ENUM "126=Init, 127=Fehler"
enum HVLM_EnergieAnfr_SocStart_Enum
{
    HVLM_ENERGIEANFR_SOCSTART_INIT = 126,
    HVLM_ENERGIEANFR_SOCSTART_FEHLER = 127
};

#define HVLM_ENERGIEANFR_SOCZIEL_ENUM "126=Init, 127=Fehler"
enum HVLM_EnergieAnfr_SocZiel_Enum
{
    HVLM_ENERGIEANFR_SOCZIEL_INIT = 126,
    HVLM_ENERGIEANFR_SOCZIEL_FEHLER = 127
};

#define HVLM_ENERGIEANFR_ZAEHLER_ENUM "14=Init, 15=Fehler"
enum HVLM_EnergieAnfr_Zaehler_Enum
{
    HVLM_ENERGIEANFR_ZAEHLER_INIT = 14,
    HVLM_ENERGIEANFR_ZAEHLER_FEHLER = 15
};

#define HVLM_LADEGRENZEANFR_LEISTUNG_ENUM "4094=Init, 4095=Fehler"
enum HVLM_LadegrenzeAnfr_Leistung_Enum
{
    HVLM_LADEGRENZEANFR_LEISTUNG_INIT = 4094,
    HVLM_LADEGRENZEANFR_LEISTUNG_FEHLER = 4095
};

#define HVLM_LADEGRENZEANFR_ZAEHLER_ENUM "14=Init, 15=Fehler"
enum HVLM_LadegrenzeAnfr_Zaehler_Enum
{
    HVLM_LADEGRENZEANFR_ZAEHLER_INIT = 14,
    HVLM_LADEGRENZEANFR_ZAEHLER_FEHLER = 15
};

#define LG_KOMPSCHUTZ_ENUM "0=inaktiv, 1=aktiv"
enum LG_KompSchutz_Enum
{
    LG_KOMPSCHUTZ_INAKTIV = 0,
    LG_KOMPSCHUTZ_AKTIV = 1
};

#define LG_ABSCHALTSTUFE_ENUM "0=keine_Einschraenkung, 1=Funktionseinschraenkung"
enum LG_Abschaltstufe_Enum
{
    LG_ABSCHALTSTUFE_KEINE_EINSCHRAENKUNG = 0,
    LG_ABSCHALTSTUFE_FUNKTIONSEINSCHRAENKUNG = 1
};

#define LG_TRANSPORT_MODE_ENUM "0=keine_Einschraenkung, 1=Funktionseinschraenkung"
enum LG_Transport_Mode_Enum
{
    LG_TRANSPORT_MODE_KEINE_EINSCHRAENKUNG = 0,
    LG_TRANSPORT_MODE_FUNKTIONSEINSCHRAENKUNG = 1
};

#define LG_NACHLAUFTYP_ENUM "0=Komm_bei_KL15_EIN, 1=Komm_nach_KL15_AUS, 2=Komm_bei_KL15_AUS"
enum LG_Nachlauftyp_Enum
{
    LG_NACHLAUFTYP_KOMM_BEI_KL15_EIN = 0,
    LG_NACHLAUFTYP_KOMM_NACH_KL15_AUS = 1,
    LG_NACHLAUFTYP_KOMM_BEI_KL15_AUS = 2
};

#define LG_SNI_ENUM "68=LG"
enum LG_SNI_Enum
{
    LG_SNI_LG = 68
};

#define KN_LADEGERAET_ECUKNOCKOUTTIMER_ENUM "63=ECUKnockOut_deaktiviert"
enum KN_Ladegeraet_ECUKnockOutTimer_Enum
{
    KN_LADEGERAET_ECUKNOCKOUTTIMER_DEAKTIVIERT = 63
};

#define KN_LADEGERAET_BUSKNOCKOUT_ENUM "0=Funktion_nicht_ausgeloest, 1=Veto_aktiv, 2=Funktion_ausgeloest, 3=Funktion_deaktiviert"
enum KN_Ladegeraet_BusKnockOut_Enum
{
    KN_LADEGERAET_BUSKNOCKOUT_NICHT_AUSGELOEST = 0,
    KN_LADEGERAET_BUSKNOCKOUT_VETO_AKTIV = 1,
    KN_LADEGERAET_BUSKNOCKOUT_AUSGELOEST = 2,
    KN_LADEGERAET_BUSKNOCKOUT_DEAKTIVIERT = 3
};

#define KN_LADEGERAET_BUSKNOCKOUTTIMER_ENUM "255=BusKnockOut_deaktiviert"
enum KN_Ladegeraet_BusKnockOutTimer_Enum
{
    KN_LADEGERAET_BUSKNOCKOUTTIMER_DEAKTIVIERT = 255
};

#define NM_LADEGERAET_WAKEUP_ENUM "0=Peripherie_Wakeup__Ursache_nicht_bekannt, 1=Bus_Wakeup, 2=KL15_HW, 4=AC_Spannung_erkannt, 5=PRX_Erkennung, 6=CP_Erkennung, 7=Taste_1, 8=Taste_2, 9=Taste_3, 10=Chademo_Stecker_stecken, 11=Chademo_Startknopf_druecken, 12=Klimatisierung_Funkfernbedienung, 13=LIN_Wakeup, 14=GBT_CC1_Wakeup, 15=GBT_CC2_Wakeup, 16=CP_Erkennung_0V_auf9V, 17=CP_Erkennung_9Voder6V_auf0V, 18=CP_Erkennung_FREQ_AN, 19=CP_Erkennung_FREQ_AUS, 20=CP_Erkennung_DELTA_DUTY, 21=SW_Reset, 22=KL30_Reset"
enum NM_Ladegeraet_Wakeup_Enum
{
    NM_LADEGERAET_WAKEUP_PERIPHERIE_UNBEKANNT = 0,
    NM_LADEGERAET_WAKEUP_BUS = 1,
    NM_LADEGERAET_WAKEUP_KL15_HW = 2,
    NM_LADEGERAET_WAKEUP_AC_SPANNUNG = 4,
    NM_LADEGERAET_WAKEUP_PRX = 5,
    NM_LADEGERAET_WAKEUP_CP = 6,
    NM_LADEGERAET_WAKEUP_TASTE_1 = 7,
    NM_LADEGERAET_WAKEUP_TASTE_2 = 8,
    NM_LADEGERAET_WAKEUP_TASTE_3 = 9,
    NM_LADEGERAET_WAKEUP_CHADEMO_STECKER = 10,
    NM_LADEGERAET_WAKEUP_CHADEMO_STARTKNOPF = 11,
    NM_LADEGERAET_WAKEUP_KLIMA_FFB = 12,
    NM_LADEGERAET_WAKEUP_LIN = 13,
    NM_LADEGERAET_WAKEUP_GBT_CC1 = 14,
    NM_LADEGERAET_WAKEUP_GBT_CC2 = 15,
    NM_LADEGERAET_WAKEUP_CP_0V_AUF_9V = 16,
    NM_LADEGERAET_WAKEUP_CP_9V_6V_AUF_0V = 17,
    NM_LADEGERAET_WAKEUP_CP_FREQ_AN = 18,
    NM_LADEGERAET_WAKEUP_CP_FREQ_AUS = 19,
    NM_LADEGERAET_WAKEUP_CP_DELTA_DUTY = 20,
    NM_LADEGERAET_WAKEUP_SW_RESET = 21,
    NM_LADEGERAET_WAKEUP_KL30_RESET = 22
};

#define KN_LADEGERAET_ECUKNOCKOUT_ENUM "0=Funktion_nicht_ausgeloest, 1=Veto_war_aktiv, 2=Funktion_ausgeloest, 3=Funktion_deaktiviert"
enum KN_Ladegeraet_ECUKnockOut_Enum
{
    KN_LADEGERAET_ECUKNOCKOUT_NICHT_AUSGELOEST = 0,
    KN_LADEGERAET_ECUKNOCKOUT_VETO_WAR_AKTIV = 1,
    KN_LADEGERAET_ECUKNOCKOUT_AUSGELOEST = 2,
    KN_LADEGERAET_ECUKNOCKOUT_DEAKTIVIERT = 3
};

#define NMH_LADEGERAET_LOKALAKTIV_ENUM "0=war_nicht_lokal_aktiv, 1=war_lokal_aktiv"
enum NMH_Ladegeraet_Lokalaktiv_Enum
{
    NMH_LADEGERAET_LOKALAKTIV_NICHT_AKTIV = 0,
    NMH_LADEGERAET_LOKALAKTIV_AKTIV = 1
};

#define NMH_LADEGERAET_SUBSYSTEMAKTIV_ENUM "0=Subsystem_war_nicht_lokalaktiv, 1=war_lokalaktiv"
enum NMH_Ladegeraet_Subsystemaktiv_Enum
{
    NMH_LADEGERAET_SUBSYSTEMAKTIV_NICHT_AKTIV = 0,
    NMH_LADEGERAET_SUBSYSTEMAKTIV_AKTIV = 1
};

#define LG_KD_FEHLER_ENUM "0=kein_KD_Fehler, 1=KD_Fehler"
enum LG_KD_Fehler_Enum
{
    LG_KD_FEHLER_KEIN_FEHLER = 0,
    LG_KD_FEHLER_FEHLER = 1
};

struct VehicleStatus {
    bool locked = false;
    bool CANQuiet = false;
};

struct ChargerStatus {
    uint8_t LAD_01_CRC;                  // LAD_01 CRC
    uint8_t LAD_01_BZ;                   // LAD_01 rolling counter
    uint8_t LAD_IstModus;                // operating mode of charger
    uint16_t LAD_AC_Istspannung;         // measured AC grid voltage (RMS)
    uint16_t LAD_IstSpannung_HV;         // measured charger HV output voltage
    float LAD_IstStrom_HV;               // measured charger HV output current
    uint8_t LAD_Status_Spgfreiheit;      // charger HV voltage-free status
    int16_t LAD_Temperatur;              // charger temperature
    uint16_t LAD_Verlustleistung;        // charger power loss
    uint32_t HVLM_MaxLadeLeistung;         // maximum DC charging power
    uint16_t HVLM_MaxSpannung_DCLS;        // maximum DC charging voltage
    uint16_t HVLM_IstStrom_DCLS;           // actual DC charging current
    uint16_t HVLM_MaxStrom_DCLS;           // maximum DC charging current
    uint16_t HVLM_MinSpannung_DCLS;        // minimum DC charging voltage
    uint16_t HVLM_MinStrom_DCLS;           // minimum DC charging current
    uint8_t HVLM_Funktion_Dauer_BattKond;  // battery continuous-conditioning function
    uint8_t HVLM_BattKond_Anf;             // battery-conditioning request
    uint8_t HVLM_Dauer_Klima_02;           // cabin climate duration
    float HVLM_AWC_Sollstrom;              // requested current for AWC charging
    uint8_t HVLM_RtmWarnLadeverbindung;    // RTM charging connection warning
    uint8_t HVLM_RtmWarnLadesystem;        // RTM charging system warning
    uint8_t HVLM_RtmWarnLadestatus;        // RTM charging status warning
    uint8_t HVLM_RtmWarnLadeKommunikation; // RTM charging communication warning
    uint8_t HVLM_Ladeanzeige_Anf;          // charge-check light request
    uint8_t HVLM_Ladeanzeige_Status;       // charge-check function status
    uint16_t HVLM_Ladeanzeige_Rampzeit;    // charge-check ramp time
    uint8_t HVLM_Ladeanzeige_Intens_Heck;  // charge-check rear intensity
    uint8_t HVLM_Ladeanzeige_Intens_Front; // charge-check front intensity
    uint8_t HVLM_11_CRC;                   // HVLM_11 CRC
    uint8_t HVLM_11_BZ;                    // HVLM_11 rolling counter
    float HVLM_HVLB_SollSpannung_HVLS;     // target HVLB input voltage request
    uint16_t HVLM_HVLB_Status;             // HVLB status bits
    uint8_t HVLM_HVLB_SollModus;           // requested HVLB operating mode
    uint32_t HVLM_PlanAnfr_Leistung;       // charging plan power request
    uint8_t HVLM_PlanAnfr_Zaehler;         // charging plan request counter
    uint16_t HVLM_PlanAnfr_Dauer;          // charging plan request duration
    uint8_t HVLM_PlanAnfr_Ladeart;         // charging plan requested charge type
    uint8_t HVLM_EnergieAnfr_SocStart;     // energy request start SOC
    uint8_t HVLM_EnergieAnfr_SocZiel;      // energy request target SOC
    uint8_t HVLM_EnergieAnfr_Zaehler;      // energy request counter
    uint32_t HVLM_LadegrenzeAnfr_Leistung; // charge limit power request
    uint8_t HVLM_LadegrenzeAnfr_Zaehler;   // charge limit request counter
    uint8_t LG_KompSchutz;                 // component protection active
    uint8_t LG_Abschaltstufe;              // active shutdown-stage restriction
    uint8_t LG_Transport_Mode;             // transport mode restriction
    uint8_t LG_Nachlauftyp;                // communication run-on capability
    uint8_t LG_SNI;                        // source node identifier
    uint8_t KN_Ladegeraet_ECUKnockOutTimer;// ECU knock-out timer
    uint8_t KN_Ladegeraet_BusKnockOut;     // bus knock-out state
    uint8_t KN_Ladegeraet_BusKnockOutTimer;// bus knock-out timer
    uint8_t NM_Ladegeraet_Wakeup;          // wakeup reason
    uint8_t KN_Ladegeraet_ECUKnockOut;     // ECU knock-out state
    uint8_t NMH_Ladegeraet_Lokalaktiv;     // local activity status
    uint8_t NMH_Ladegeraet_Subsystemaktiv; // subsystem activity status
    uint8_t LG_KD_Fehler;                  // service fault present
    uint8_t HVLM_04_CRC;                   // HVLM_04 CRC
    uint8_t HVLM_04_BZ;                    // HVLM_04 rolling counter
    uint8_t HVLM_STH_Betriebsmodus;        // stand-heater operating mode
    uint8_t HVLM_Standklima_Timer_Status;  // stand climate timer status
    uint16_t HVLM_HVEM_MaxLeistung;        // max allowed power for HVEM
    uint8_t HVLM_Status_Netz;              // status if vehicle is connected to AC grid
    uint8_t HVLM_Anf_Ladescreen;           // request charging screen
    uint8_t HVLM_Ladeart;                  // charging type / energy flow
    uint8_t HVLM_VK_STH_Einsatz;           // pre-conditioning stand-heater strategy
    uint8_t HVLM_VK_Modus;                 // current pre-conditioning mode
    uint8_t HVLM_IstModus_02;              // current charging manager mode
    uint8_t HVLM_HV_Anf;                   // HV activation request and reason
    uint8_t HVLM_Fehlerstatus;             // charger error status
    uint8_t HVLM_Anforderung_HMS;          // request to lock drivetrain
    uint8_t HVLM_Parken_beibehalten_HMS;   // maintain drivetrain lock
    uint8_t HVLM_AWC_Sollmodus;            // requested AWC charger mode
    uint8_t HVLM_Stecker_Status;           // plug status
    uint8_t HVLM_LadeAnforderung;          // charging request
    uint8_t HVLM_MaxBatLadestromHV;        // recommended battery charging current
    uint16_t HVLM_HV_Abstellzeit;          // time between HV off and on
    uint8_t HVLM_Ladesystemhinweise;       // charger/system hint status
    uint8_t HVLM_Schluessel_Anfrage;       // key search request
    uint8_t HVLM_Zustand_LED;              // status of charging LED
    float HVLM_MaxStrom_Netz;              // max AC current at charger primary side
    uint8_t HVLM_LG_Sollmodus;             // charger target mode
    uint8_t HVLM_FreigabeTankdeckel;       // tank cap release request
    uint8_t HVLM_Stecker_Verriegeln;       // connector lock request
    uint8_t HVLM_Start_Spannungsmessung_DCLS; // DCLS voltage-measure start request
    uint8_t PnC_Trigger_OBC_cGW;           // plug and charge trigger
    uint8_t HVLM_FreigabeKlimatisierung;   // climate enable request
    uint8_t HVLM_Ladetexte;                // charging-text/status
    uint8_t HVLM_IsoMessung_Anf;           // isolation-measurement mode request
    uint16_t HVLM_IstSpannung_HV;          // charger output voltage / DC station voltage
    uint8_t LAD_02_CRC;                    // LAD_02 CRC
    uint8_t LAD_02_BZ;                     // LAD_02 rolling counter
    uint8_t LAD_Abregelung_Temperatur;     // reduced due to charger temperature
    uint8_t LAD_Abregelung_IU_Ein_Aus;     // reduced due to current or voltage
    uint8_t LAD_Abregelung_BuchseTemp;     // reduced due to socket temperature
    uint16_t LAD_MaxLadLeistung_HV;        // max charger power
    uint8_t LAD_PRX_Stromlimit;            // AC current limit from PRX cable
    uint8_t LAD_CP_Erkennung;              // status of control pilot monitoring
    uint8_t LAD_Stecker_Verriegelt;        // connector lock feedback
    uint8_t LAD_Kuehlbedarf;               // cooling demand of charger
    uint8_t LAD_MaxLadLeistung_HV_Offset;  // optional +25W offset steps
    uint8_t LAD_Warnzustand;               // warning flag
    uint8_t LAD_Fehlerzustand;             // fault flag
};

struct ChargerControl {
    uint16_t HVDCSetpnt;
    uint16_t IDCSetpnt;
    uint16_t HVpwr = 0;
    uint16_t HVcur = 0;
    uint16_t calcpwr = 0;
    bool activate;
    uint8_t HVActiveDelayOff;
};

struct BatteryStatus {
    uint16_t SOCx10;         // SOC of battery, with implied decimal place
    uint16_t SOC_Targetx10;  // target SOC of battery, with implied decimal place
    uint16_t CapkWhx10 = 365;      // usable energy content of the HV battery
    uint16_t BattkWhx10 = 273;     // current energy content of the HV battery
    uint16_t BMSVoltx10 = 4100;     // BMS voltage of battery
    uint16_t BMSCurrx10 =1256;     // BMS current of battery, with implied decimal place
    uint16_t BMSMaxVolt=400;     // BMS maximum battery voltage
    uint16_t BMSMinVolt=0;     // BMS minimum battery voltage
    uint16_t BMSMaxChargeCurr=300;  // BMS maximum charge current
    uint16_t BMSBattCellSumx10=0;
    uint16_t BMSCellAhx10 = 1080;
    uint8_t HV_Status;       // 0=Init (no function), 1=BMS HV free (<20V), 2=BMS HV active (>=25V), 3=Error
    uint8_t BMS_Status;      // 0 "Component_OK" 1 "Limited_CompFct_Isoerror_I" 2 "Limited_CompFct_Isoerror_II" 3 "Limited_CompFct_Interlock" 4 "Limited_CompFct_SD" 5 "Limited_CompFct_Powerred" 6 "No_component_function" 7 "Init"
    uint8_t BMS_Mode;        // 0 "HV_Off" 1 "Driving HV Active" 2 "Balancing" 3 "External Charger" 4 "AC Charging" 5 "Battery Fault" 6 "DC Charging" 7 "Init"
    uint16_t BMS_Battery_Tempx10 = 242;
    uint16_t BMS_Coolant_Tempx10 = 201;
    uint16_t BMS_Cell_H_Tempx10 = 290;
    uint16_t BMS_Cell_L_Tempx10 = 220;
    uint16_t BMS_Cell_H_mV = 3850;
    uint16_t BMS_Cell_L_mV = 3750;
    bool HVIL_Open = false;
};

struct MLB_State {
    uint32_t UnixTime{};
    uint16_t BMS_Batt_Curr{};
    uint16_t BMS_Batt_Volt{};
    uint16_t BMS_Batt_Volt_HVterm{};
    uint16_t BMS_SOC_HiRes{};
    uint16_t BMS_MaxDischarge_Curr{};
    uint16_t BMS_Min_Batt_Volt{};
    uint16_t BMS_Min_Batt_Volt_Discharge{};
    uint16_t BMS_MaxCharge_Curr{};
    uint16_t BMS_MaxCharge_Curr_Offset{};
    uint16_t BMS_Batt_Max_Volt{};
    uint16_t BMS_Min_Batt_Volt_Charge{};
    uint16_t BMS_OpenCircuit_Volts{};
    bool BMS_Status_ServiceDisconnect{};
    uint8_t BMS_HV_Status{};
    bool BMS_Faultstatus{};
    uint8_t BMS_IstModus{};
    uint16_t BMS_Batt_Ah{};
    uint16_t BMS_Target_SOC_HiRes{};
    uint16_t BMS_Batt_Temp{};
    uint16_t BMS_CurrBatt_Temp{};
    uint16_t BMS_CoolantTemp_Act{};
    uint16_t BMS_Batt_Energy{};
    uint16_t BMS_Max_Wh{};
    uint16_t BMS_BattEnergy_Wh_HiRes{};
    uint16_t BMS_MaxBattEnergy_Wh_HiRes{};
    uint16_t BMS_SOC{};
    uint16_t BMS_ResidualEnergy_Wh{};
    uint16_t BMS_SOC_ChargeLim{};
    uint16_t BMS_EnergyCount{};
    uint16_t BMS_EnergyReq_Full{};
    uint16_t BMS_ChargePowerMax{};
    uint16_t BMS_ChargeEnergyCount{};
    uint16_t BMS_BattCell_Temp_Max{};
    uint16_t BMS_BattCell_Temp_Min{};
    uint16_t BMS_BattCell_MV_Max{};
    uint16_t BMS_BattCell_MV_Min{};
    bool HVEM_Nachladen_Anf{};
    uint16_t HVEM_SollStrom_HV{};
    uint16_t HVEM_MaxSpannung_HV{};
    uint8_t HMS_Systemstatus{};
    uint8_t HMS_aktives_System{};
    bool HMS_Fehlerstatus{};
    uint8_t HVK_HVLM_Sollmodus{}; // requested target mode of the charging manager: 0=Not Enabled, 1=Enabled
    bool HV_Bordnetz_aktiv{};     // high-voltage vehicle electrical system active: 0=Not Active, 1=Active
    uint8_t HVK_MO_EmSollzustand{}; // 0 "HvOff" 1 "HvStbyReq" 2 "HvStbyWait" 3 "HvBattOnReq" 4 "HvBattOnWait" 10 "HvOnIdle" 20 "HvOnDrvRdy" 46 "HvAcChPreReq" 47 "HvAcChPreWait" 48 "HvAcChReq" 49 "HvDcChWait" 50 "HvDcCh" 56 "HvDcChPreReq" 57 "HvDcChPreWait" 58 "HvDcChReq" 59 "HvDcChWait" 60 "HvDcCh" 67 "HvChOffReq" 68 "HvChOffWait" 69 "HvOnIdleReq" 70 "HvOnIdleWait" 96 "HvCpntOffReq" 97 "HvCpntOffWait" 98 "HvBattOffReq" 99 "HvBattOffWait" 119 "HvElmOffReq" 120 "HvElmOff"
    uint8_t HVK_BMS_Sollmodus{};    // BMS requested mode: 0 "HV_Off" 1 "HV_On" 3 "AC_Charging_ext" 4 "AC_Charging" 6 "DC_Charging" 7 "Init"
    uint8_t HVK_DCDC_Sollmodus{};   // DC/DC requested mode: 0 "Standby" 1 "HV_On_Precharging" 2 "Step down" 3 "Step up" 4 "Test pulse_12V" 7 "Initialization"
    bool ZV_FT_verriegeln{};
    bool ZV_FT_entriegeln{};
    bool ZV_BT_verriegeln{};
    bool ZV_BT_entriegeln{};
    bool ZV_entriegeln_Anf{};
    bool ZV_verriegelt_intern_ist{};
    bool ZV_verriegelt_extern_ist{};
    bool ZV_verriegelt_intern_soll{};
    bool ZV_verriegelt_extern_soll{};
    uint8_t ZV_verriegelt_soll{};
    bool BMS_Charger_Active{};
    uint16_t BMS_RIso_Ext{4090};
    uint8_t HVK_Gesamtst_Spgfreiheit{};
    uint8_t BMS_Balancing_Active{2};
    uint8_t BMS_Freig_max_Perf{1};
    uint8_t BMS_Battdiag{1};       // battery display diagnostics: 1=Display Battery, 4=Battery OK, 5=Charging, 6=Check Battery
    uint8_t DC_IstModus_02{2};
    uint8_t BMS_HV_Auszeit_Status{1}; // status HV timeout
    uint16_t BMS_HV_Auszeit{25};      // time since last HV activity
    uint16_t BMS_Kapazitaet{1000};    // total energy capacity (aged)
    uint16_t BMS_SOC_Kaltstart{};     // SOC cold
    uint8_t BMS_max_Grenz_SOC{30};    // upper limit of SOC operating strategy (70 offset -> 30 = 100)
    uint8_t BMS_min_Grenz_SOC{15};    // lower limit of SOC operating strategy
    uint8_t EM1_Status_Spgfreiheit{};    // voltage status: 0=Init, 1=NoVoltage, 2=Voltage, 3=Fault & Voltage
    bool ZAS_Kl_S{};                    // key switch inserted
    bool ZAS_Kl_15{};                   // accessory position
    bool ZAS_Kl_X{};                    // run position
    bool ZAS_Kl_50_Startanforderung{};   // start request
    uint8_t HVEM_NVNachladen_Energie{200};
};

class VWMLBClass: public Chargerhw
{
public:
      bool ControlCharge(bool RunCh, bool ACReq);
      void SetCanInterface(CanHardware*);
      void DecodeCAN(int id, uint32_t data[2]);
      void Task10Ms();
      void Task100Ms();


private:
      static constexpr uint32_t ID_ZV_01  = 0x184;
      static constexpr uint32_t ID_BMS_01 = 0x191;
      static constexpr uint32_t ID_BMS_02 = 0x1A1;
      static constexpr uint32_t ID_BMS_03 = 0x39D;
      static constexpr uint32_t ID_DCDC_01 = 0x2AE;
      static constexpr uint32_t ID_BMS_10 = 0x509;
      static constexpr uint32_t ID_HVK_01 = 0x503;
      static constexpr uint32_t ID_BMS_DC_01 = 0x578;
      static constexpr uint32_t ID_HVEM_05 = 0x552;
      static constexpr uint32_t ID_ZV_02  = 0x583;
      static constexpr uint32_t ID_BMS_04 = 0x5A2;
      static constexpr uint32_t ID_BMS_06 = 0x59E;
      static constexpr uint32_t ID_BMS_07 = 0x5CA;
      static constexpr uint32_t ID_DCDC_03 = 0x5CD;
      static constexpr uint32_t ID_HVEM_02 = 0x5AC;
      static constexpr uint32_t ID_NAVDATA_02 = 0x485;
      static constexpr uint32_t ID_ORU_01 = 0x1A555548;
      static constexpr uint32_t ID_AUTHENTIC_TIME_01 = 0x1A5555AD;
      static constexpr uint32_t ID_BMS_09 = 0x96A955EB;
      static constexpr uint32_t ID_BMS_11 = 0x96A954A6;
      static constexpr uint32_t ID_BMS_16 = 0x9A555539;
      static constexpr uint32_t ID_BMS_27 = 0x9A555552;
      static constexpr uint32_t ID_ESP_15 = 0x1A2;
      //void CommandStates();
      void TagParams();
      void CalcValues100ms();
      void msg3C0();
      void msg1A1();      // BMS_02     0x1A1
      void msg2B1();      // MSG_TME_02   0x2B1
      void msg39D();      // BMS_03     0x39D
      void msg485();      // NavData_02 0x485
      void msg509();      // BMS_10     0x509
      void msg552();      // HVEM_05    0x552
      void msg583();      // ZV_02      0x583
      void msg59E();      // BMS_06     0x59E
      void msg5AC();      // HVEM_02    0x5AC
      void msg64F();      // BCM1_04    0x64F
      void msg663();      // NVEM_02    0x663
      void msg1A555548(); // ORU_01     0x1A555548
      void msg1A5555AD(); // Authentic_Time_01   0x1A5555AD
      void msg96A955EB(); // BMS_09     0x96A955EB
      void msg96A954A6(); // BMS_11     0x96A954A6
      void msg9A555539(); // BMS_16     0x9A555539
      void msg9A555552(); // BMS_27     0x9A555552
      void msg040();      // Airbag_01  0x40
      void msg184();      // ZV_01      0x184
      void msg191();      // BMS_01     0x191
      void msg1A2();      // ESP_15   0x1A2
      void msg2AE();      // DCDC_01    0x2AE
      void msg37C();      // EM1_HYB_11    0x37C
      void msg503();      // HVK_01     0x503
      void msg578();      // BMS_DC_01  0x578
      void msg5A2();      // BMS_04     0x5A2
      void msg5CA();      // BMS_07     0x5CA
      void msg5CD();      // DCDC_03    0x5CD
      uint8_t vag_cnt3C0            = 0x00;
      uint8_t vag_cnt184            = 0x00;
      uint8_t vag_cnt191            = 0x00;
      uint8_t vag_cnt1A2            = 0x00;
      uint8_t vag_cnt2AE            = 0x00;
      uint8_t vag_cnt503            = 0x00;
      uint8_t vag_cnt578            = 0x00;
      uint8_t vag_cnt5A2            = 0x00;
      uint8_t vag_cnt5CA            = 0x00;
      uint8_t vag_cnt5CD            = 0x00;

      VehicleStatus vehicle_status;
      ChargerStatus charger_status;
      ChargerControl charger_params;
      BatteryStatus battery_status;

      MLB_State mlb_state{};

      void emulateMLB();

      
};

#endif /* vw_mlb_charger_h */
